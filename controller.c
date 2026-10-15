#include "controller.h"
#include "option_pricing.h"
#include "worker.h"
#include "stats.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

/* Split total_sims across num_workers as evenly as possible.
 * First (remainder) workers get one extra path each. */
static void distribute_work(int total_sims, int num_workers,
                             int *sims_per_worker) {
    int base      = total_sims / num_workers;
    int remainder = total_sims % num_workers;
    for (int i = 0; i < num_workers; i++)
        sims_per_worker[i] = base + (i < remainder ? 1 : 0);
}

int run_engine(const engine_config_t *config,
               const model_params_t  *params,
               engine_result_t       *result) {
    int nw         = config->num_workers;
    int total_sims = config->num_sims;

    int (*ctrl_to_worker)[2] = xmalloc((size_t)nw * sizeof(int[2]));
    int (*worker_to_ctrl)[2] = xmalloc((size_t)nw * sizeof(int[2]));
    int   *sims_per_worker   = xmalloc((size_t)nw * sizeof(int));
    pid_t *pids              = xmalloc((size_t)nw * sizeof(pid_t));

    distribute_work(total_sims, nw, sims_per_worker);

    /* Create all pipes before forking so every child inherits all fds.
     * We then selectively close the ones each process doesn't own. */
    for (int i = 0; i < nw; i++) {
        if (pipe(ctrl_to_worker[i]) != 0) die("pipe(ctrl_to_worker)");
        if (pipe(worker_to_ctrl[i]) != 0) die("pipe(worker_to_ctrl)");
    }

    for (int i = 0; i < nw; i++) {
        pid_t pid = fork();
        if (pid < 0) die("fork");

        if (pid == 0) {
            /* Child: keep only ctrl_to_worker[i][0] and worker_to_ctrl[i][1].
             * Close everything else — leaving extra write ends open would
             * prevent the controller from ever seeing EOF on those pipes. */
            for (int j = 0; j < nw; j++) {
                if (j == i) {
                    close(ctrl_to_worker[j][1]);
                    close(worker_to_ctrl[j][0]);
                } else {
                    close(ctrl_to_worker[j][0]);
                    close(ctrl_to_worker[j][1]);
                    close(worker_to_ctrl[j][0]);
                    close(worker_to_ctrl[j][1]);
                }
            }
            run_worker(ctrl_to_worker[i][0], worker_to_ctrl[i][1]);
        }

        pids[i] = pid;
    }

    /* Parent: close the child-facing ends we don't use. */
    for (int i = 0; i < nw; i++) {
        close(ctrl_to_worker[i][0]);
        close(worker_to_ctrl[i][1]);
    }

    /* Send each worker its task.
     *
     * Per-worker seed: base_seed XOR (i * 2654435761)
     * 2654435761 is Knuth's multiplicative hash constant (~2^32/phi).
     * XOR with the index spreads seeds well and avoids seed=0 for any worker. */
    const uint32_t KNUTH_CONST = 2654435761u;
    int send_ok = 1;

    for (int i = 0; i < nw; i++) {
        if (!send_ok) {
            close(ctrl_to_worker[i][1]);
            continue;
        }

        simulation_task_t task = {
            .worker_id = i,
            .num_sims  = sims_per_worker[i],
            .seed      = config->base_seed ^ (uint32_t)((unsigned)i * KNUTH_CONST),
            .params    = *params
        };

        if (write_all(ctrl_to_worker[i][1], &task, sizeof(task)) != 0) {
            fprintf(stderr, "controller: failed to send task to worker %d\n", i);
            send_ok = 0;
        }
        /* Close write end so the worker's read_all() sees EOF and returns. */
        close(ctrl_to_worker[i][1]);
    }

    /* Collect results with select() so we drain whichever worker finishes
     * first, not necessarily worker 0. Because arrival order is unknown,
     * each worker's PnL goes into a pre-computed fixed slice (offset_for[i])
     * rather than a running offset. */
    double *all_pnl    = xmalloc((size_t)total_sims * sizeof(double));
    int    *offset_for = xmalloc((size_t)nw * sizeof(int));
    int    *finished   = calloc((size_t)nw, sizeof(int));
    if (!finished) die("calloc");

    {
        int off = 0;
        for (int i = 0; i < nw; i++) {
            offset_for[i] = off;
            off += sims_per_worker[i];
        }
    }

    int workers_done = 0;
    int collect_ok   = send_ok;

    if (!collect_ok) {
        for (int i = 0; i < nw; i++) {
            close(worker_to_ctrl[i][0]);
            finished[i] = 1;
        }
        workers_done = nw;
    }

    while (workers_done < nw) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;

        for (int i = 0; i < nw; i++) {
            if (!finished[i]) {
                FD_SET(worker_to_ctrl[i][0], &readfds);
                if (worker_to_ctrl[i][0] > maxfd)
                    maxfd = worker_to_ctrl[i][0];
            }
        }

        int nready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue;
            perror("select");
            collect_ok = 0;
            for (int i = 0; i < nw; i++) {
                if (!finished[i]) {
                    close(worker_to_ctrl[i][0]);
                    finished[i] = 1;
                    workers_done++;
                }
            }
            break;
        }

        for (int i = 0; i < nw; i++) {
            if (finished[i] || !FD_ISSET(worker_to_ctrl[i][0], &readfds))
                continue;

            worker_result_header_t header;
            if (read_all(worker_to_ctrl[i][0], &header, sizeof(header)) != 0) {
                fprintf(stderr, "controller: failed to read header from worker %d\n", i);
                collect_ok = 0;
                close(worker_to_ctrl[i][0]);
                finished[i] = 1;
                workers_done++;
                continue;
            }

            if (header.worker_id != i || header.num_sims != sims_per_worker[i]) {
                fprintf(stderr,
                        "controller: protocol mismatch from worker %d "
                        "(expected id=%d sims=%d, got id=%d sims=%d)\n",
                        i, i, sims_per_worker[i],
                        header.worker_id, header.num_sims);
                collect_ok = 0;
                close(worker_to_ctrl[i][0]);
                finished[i] = 1;
                workers_done++;
                continue;
            }

            size_t pnl_bytes = (size_t)header.num_sims * sizeof(double);
            if (read_all(worker_to_ctrl[i][0],
                         all_pnl + offset_for[i], pnl_bytes) != 0) {
                fprintf(stderr, "controller: failed to read PnL from worker %d\n", i);
                collect_ok = 0;
            }

            close(worker_to_ctrl[i][0]);
            finished[i] = 1;
            workers_done++;
        }
    }

    free(finished);
    free(offset_for);

    int rc;
    if (collect_ok) {
        result->metrics  = compute_risk_metrics(all_pnl, total_sims);
        result->price_t0 = bs_call_price(params->S0, params->K,
                                          params->r, params->sigma, params->T);
        rc = 0;
    } else {
        rc = -1;
    }

    free(all_pnl);

    /* Reap children — always, even on error, to avoid zombie processes. */
    for (int i = 0; i < nw; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "controller: worker %d exited abnormally\n", i);
        }
    }

    free(pids);
    free(sims_per_worker);
    free(worker_to_ctrl);
    free(ctrl_to_worker);

    return rc;
}
