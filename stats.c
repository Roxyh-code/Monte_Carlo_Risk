#include "stats.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static int compare_double_asc(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

void sort_doubles(double *data, int n) {
    qsort(data, (size_t)n, sizeof(double), compare_double_asc);
}

double compute_mean(const double *data, int n) {
    if (n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += data[i];
    return sum / (double)n;
}

double compute_stddev(const double *data, int n, double mean) {
    if (n <= 1) return 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        double d = data[i] - mean;
        sum_sq += d * d;
    }
    return sqrt(sum_sq / (double)(n - 1));  /* Bessel's correction */
}

double empirical_quantile(const double *sorted_data, int n, double p) {
    if (n <= 0) return 0.0;
    int idx = (int)(p * (double)n);
    if (idx < 0)  idx = 0;
    if (idx >= n) idx = n - 1;
    return sorted_data[idx];
}

risk_metrics_t compute_risk_metrics(double *pnl, int n) {
    risk_metrics_t m;
    sort_doubles(pnl, n);
    m.mean   = compute_mean(pnl, n);
    m.stddev = compute_stddev(pnl, n, m.mean);
    m.var_95 = -empirical_quantile(pnl, n, 0.05);
    m.var_99 = -empirical_quantile(pnl, n, 0.01);
    return m;
}
