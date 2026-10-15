#ifndef STATS_H
#define STATS_H

/*
 * VaR sign convention: VaR is a POSITIVE number representing a potential loss.
 *   VaR_95 = -(5th percentile of PnL)
 *   VaR_99 = -(1st percentile of PnL)
 * Industry standard (Basel III). Positive VaR = "you could lose this much."
 */
typedef struct {
    double mean;
    double stddev;
    double var_95;
    double var_99;
} risk_metrics_t;

void           sort_doubles(double *data, int n);
double         compute_mean(const double *data, int n);
double         compute_stddev(const double *data, int n, double mean);
double         empirical_quantile(const double *sorted_data, int n, double p);
risk_metrics_t compute_risk_metrics(double *pnl, int n);  /* sorts pnl in place */

#endif /* STATS_H */
