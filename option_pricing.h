#ifndef OPTION_PRICING_H
#define OPTION_PRICING_H

/* Standard normal CDF via erfc(): Phi(x) = 0.5 * erfc(-x / sqrt(2)) */
double norm_cdf(double x);

/* Black-Scholes European call price. Returns intrinsic value if T <= 0. */
double bs_call_price(double S, double K, double r, double sigma, double T);

#endif /* OPTION_PRICING_H */
