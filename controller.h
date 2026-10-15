#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include "protocol.h"
#include "stats.h"

typedef struct {
    int      num_workers;
    int      num_sims;
    uint32_t base_seed;
} engine_config_t;

typedef struct {
    risk_metrics_t metrics;
    double         price_t0;   /* BS call price at t=0, printed in output */
} engine_result_t;

/* Fork workers, run simulations, aggregate results. Returns 0 on success. */
int run_engine(const engine_config_t *config,
               const model_params_t  *params,
               engine_result_t       *result);

#endif /* CONTROLLER_H */
