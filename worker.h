#ifndef WORKER_H
#define WORKER_H

/*
 * Entry point for child processes after fork().
 * Reads task from task_read_fd, runs simulations, writes results to
 * result_write_fd, then exits. Never returns.
 */
void run_worker(int task_read_fd, int result_write_fd);

#endif /* WORKER_H */
