#ifndef MC_H
#define MC_H

#include "protocol.h"

/*
 * GBM simulation for 1-day option PnL:
 *   S1 = S0 * exp((r - sigma^2/2)*dt + sigma*sqrt(dt)*Z),  Z ~ N(0,1)
 *   PnL = bs_call_price(S1, ..., T-dt) - bs_call_price(S0, ..., T)
 */

double sample_standard_normal(unsigned int *seed);
void   run_simulations(const simulation_task_t *task, double *pnl_out);

#endif /* MC_H */
