#include "option_pricing.h"

#include <math.h>

double norm_cdf(double x) {
    /* erfc is numerically stable for all finite x, unlike a raw series. */
    return 0.5 * erfc(-x / M_SQRT2);
}

double bs_call_price(double S, double K, double r, double sigma, double T) {
    if (T <= 0.0) {
        double intrinsic = S - K;
        return intrinsic > 0.0 ? intrinsic : 0.0;
    }

    double sqrt_T = sqrt(T);
    double d1 = (log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    double d2 = d1 - sigma * sqrt_T;

    return S * norm_cdf(d1) - K * exp(-r * T) * norm_cdf(d2);
}
