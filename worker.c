#include "worker.h"
#include "protocol.h"
#include "mc.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void run_worker(int task_read_fd, int result_write_fd) {
    /* Read the task sent by the controller. */
    simulation_task_t task;
    if (read_all(task_read_fd, &task, sizeof(task)) != 0) {
        fprintf(stderr, "worker: failed to read task\n");
        exit(EXIT_FAILURE);
    }
    close(task_read_fd);

    /* Run Monte Carlo simulations. */
    double *pnl = xmalloc((size_t)task.num_sims * sizeof(double));
    run_simulations(&task, pnl);

    /* Send header first so the controller knows how many doubles to expect. */
    worker_result_header_t header = {
        .worker_id = task.worker_id,
        .num_sims  = task.num_sims
    };
    if (write_all(result_write_fd, &header, sizeof(header)) != 0) {
        fprintf(stderr, "worker %d: failed to write header\n", task.worker_id);
        free(pnl);
        exit(EXIT_FAILURE);
    }

    /* Stream PnL data back. */
    size_t pnl_bytes = (size_t)task.num_sims * sizeof(double);
    if (write_all(result_write_fd, pnl, pnl_bytes) != 0) {
        fprintf(stderr, "worker %d: failed to write PnL\n", task.worker_id);
        free(pnl);
        exit(EXIT_FAILURE);
    }

    free(pnl);
    close(result_write_fd);
    exit(EXIT_SUCCESS);
}
