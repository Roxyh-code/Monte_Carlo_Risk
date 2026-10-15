/*
 * protocol.h - IPC message structs shared between controller and workers.
 *
 * Each worker gets two pipes:
 *   ctrl_to_worker[i]: controller sends one simulation_task_t
 *   worker_to_ctrl[i]: worker sends worker_result_header_t + double[] PnL
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* Market and option parameters passed to every worker. */
typedef struct {
    double S0;      /* current underlying price          */
    double K;       /* strike price                      */
    double r;       /* annualised risk-free rate         */
    double sigma;   /* annualised volatility             */
    double T;       /* time to maturity (years)          */
    double dt;      /* simulation step = 1/252 years     */
} model_params_t;

/* Sent by controller to each worker before simulation starts. */
typedef struct {
    int          worker_id;
    int          num_sims;
    uint32_t     seed;
    model_params_t params;
} simulation_task_t;

/*
 * First message worker sends back. Immediately followed by
 * (num_sims * sizeof(double)) bytes of raw PnL data.
 */
typedef struct {
    int worker_id;
    int num_sims;
} worker_result_header_t;

#endif /* PROTOCOL_H */
