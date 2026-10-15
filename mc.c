#include "mc.h"
#include "option_pricing.h"

#include <math.h>
#include <stdlib.h>

double sample_standard_normal(unsigned int *seed) {
    /* Box-Muller: two uniform samples → one standard normal.
     * Shift away from 0 to avoid log(0) if rand_r returns 0. */
    double u1 = ((double)rand_r(seed) + 1.0) / ((double)RAND_MAX + 2.0);
    double u2 = ((double)rand_r(seed) + 1.0) / ((double)RAND_MAX + 2.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

void run_simulations(const simulation_task_t *task, double *pnl_out) {
    const model_params_t *p    = &task->params;
    unsigned int          seed = (unsigned int)task->seed;

    /* price_t0 is the same for all paths — compute it once outside the loop. */
    double price_t0 = bs_call_price(p->S0, p->K, p->r, p->sigma, p->T);
    double T1       = p->T - p->dt;
    if (T1 < 0.0) T1 = 0.0;

    double drift     = (p->r - 0.5 * p->sigma * p->sigma) * p->dt;
    double diffusion = p->sigma * sqrt(p->dt);

    for (int i = 0; i < task->num_sims; i++) {
        double Z        = sample_standard_normal(&seed);
        double S1       = p->S0 * exp(drift + diffusion * Z);
        double price_t1 = bs_call_price(S1, p->K, p->r, p->sigma, T1);
        pnl_out[i]      = price_t1 - price_t0;
    }
}
