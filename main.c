/*
 * main.c — Entry point: argument parsing, validation, timing, output.
 *
 * Usage:
 *   ./risk_engine <num_workers> <num_sims> <S0> <K> <r> <sigma> <T_years> [seed]
 *
 * The seed argument is optional.  When omitted, a fixed default (20240101)
 * is used, making runs fully reproducible.  Supplying a different seed
 * produces a statistically independent but equally valid set of paths,
 * which is useful for:
 *   - Verifying that VaR estimates are stable across seeds
 *   - Exploring Monte Carlo sampling variance
 *   - Running independent replicas in a benchmark
 *
 * Examples:
 *   ./risk_engine 4 50000 100 100 0.05 0.2 0.25
 *   ./risk_engine 4 50000 100 100 0.05 0.2 0.25 42
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "controller.h"
#include "protocol.h"

/* -------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

#define TRADING_DAYS_PER_YEAR  252
#define MIN_WORKERS            3
#define DEFAULT_SEED           20240101u

/* -------------------------------------------------------------------------
 * Usage / help
 * ------------------------------------------------------------------------- */

static void print_usage(const char *prog) {
    fprintf(stderr,
        "\nUsage:\n"
        "  %s <num_workers> <num_sims> <S0> <K> <r> <sigma> <T_years> [seed]\n"
        "\n"
        "Arguments:\n"
        "  num_workers   Worker processes to fork (minimum %d)\n"
        "  num_sims      Total Monte Carlo simulations (> 0)\n"
        "  S0            Current underlying price       (> 0)\n"
        "  K             Option strike price            (> 0)\n"
        "  r             Annualised risk-free rate      (e.g. 0.05)\n"
        "  sigma         Annualised volatility          (> 0, e.g. 0.20)\n"
        "  T_years       Time to maturity in years      (> 1 trading day)\n"
        "  seed          Base PRNG seed (optional, default %u)\n"
        "\n"
        "Examples:\n"
        "  %s 4 50000 100 100 0.05 0.2 0.25\n"
        "  %s 4 50000 100 100 0.05 0.2 0.25 42\n\n",
        prog, MIN_WORKERS, DEFAULT_SEED, prog, prog);
}

/* -------------------------------------------------------------------------
 * Formatted output — called after a successful engine run.
 * ------------------------------------------------------------------------- */

static void print_results(const engine_config_t *config,
                           const model_params_t  *params,
                           const engine_result_t *result,
                           double                 elapsed_sec) {
    printf("\n");
    printf("=================================================================\n");
    printf("         Parallel Monte Carlo Risk Engine  —  Results            \n");
    printf("=================================================================\n");
    printf("  Execution\n");
    printf("    Worker processes          : %d\n",     config->num_workers);
    printf("    Total simulations         : %d\n",     config->num_sims);
    printf("    Base seed                 : %u\n",     config->base_seed);
    printf("    Wall-clock runtime        : %.4f s\n", elapsed_sec);
    printf("-----------------------------------------------------------------\n");
    printf("  Model  (European Call, Black-Scholes / Risk-Neutral GBM)\n");
    printf("    Underlying price    S0    : %8.4f\n",  params->S0);
    printf("    Strike              K     : %8.4f\n",  params->K);
    printf("    Risk-free rate      r     : %8.4f\n",  params->r);
    printf("    Volatility          sigma : %8.4f\n",  params->sigma);
    printf("    Maturity            T     : %8.4f  years\n", params->T);
    printf("    Horizon             dt    : %8.6f  years  [1/252 trading day]\n",
           params->dt);
    printf("    Call price at t=0  (BS)  : %8.4f\n",  result->price_t0);
    printf("-----------------------------------------------------------------\n");
    printf("  Risk Metrics  (1-day PnL, long call position)\n");
    printf("    Mean PnL                  : %+10.6f\n", result->metrics.mean);
    printf("    Std Dev of PnL            : %10.6f\n",  result->metrics.stddev);
    printf("    95%% 1-day VaR            : %10.6f"
           "  [max loss w/ 95%% confidence]\n", result->metrics.var_95);
    printf("    99%% 1-day VaR            : %10.6f"
           "  [max loss w/ 99%% confidence]\n", result->metrics.var_99);
    printf("=================================================================\n\n");
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    /* Accept 7 required arguments + 1 optional seed. */
    if (argc != 8 && argc != 9) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Parse required arguments.
     * atoi/atof produce 0 for invalid strings; validation below catches them. */
    int    num_workers = atoi(argv[1]);
    int    num_sims    = atoi(argv[2]);
    double S0          = atof(argv[3]);
    double K           = atof(argv[4]);
    double r           = atof(argv[5]);
    double sigma       = atof(argv[6]);
    double T           = atof(argv[7]);

    /* Parse optional seed argument.
     * strtoul is used instead of atoi so that very large seed values
     * (e.g. 0xDEADBEEF) are parsed correctly as unsigned integers. */
    uint32_t base_seed = DEFAULT_SEED;
    if (argc == 9) {
        unsigned long parsed = strtoul(argv[8], NULL, 0);
        base_seed = (uint32_t)parsed;
    }

    /* Simulation step: 1 trading day expressed in years. */
    double dt = 1.0 / (double)TRADING_DAYS_PER_YEAR;

    /* Validate — collect all errors before exiting so the user sees every
     * problem at once rather than fixing one at a time. */
    int valid = 1;

    if (num_workers < MIN_WORKERS) {
        fprintf(stderr, "Error: num_workers must be >= %d (got %d)\n",
                MIN_WORKERS, num_workers);
        valid = 0;
    }
    if (num_sims <= 0) {
        fprintf(stderr, "Error: num_sims must be > 0 (got %d)\n", num_sims);
        valid = 0;
    }
    if (S0 <= 0.0) {
        fprintf(stderr, "Error: S0 must be > 0 (got %.4f)\n", S0);
        valid = 0;
    }
    if (K <= 0.0) {
        fprintf(stderr, "Error: K must be > 0 (got %.4f)\n", K);
        valid = 0;
    }
    if (sigma <= 0.0) {
        fprintf(stderr, "Error: sigma must be > 0 (got %.4f)\n", sigma);
        valid = 0;
    }
    if (T <= dt) {
        fprintf(stderr,
                "Error: T_years must be > %.6f (1 trading day); got %.6f\n",
                dt, T);
        valid = 0;
    }

    if (!valid) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Build configuration and parameter structs. */
    engine_config_t config = {
        .num_workers = num_workers,
        .num_sims    = num_sims,
        .base_seed   = base_seed
    };

    model_params_t params = {
        .S0    = S0,
        .K     = K,
        .r     = r,
        .sigma = sigma,
        .T     = T,
        .dt    = dt
    };

    /*
     * Time the engine run using CLOCK_MONOTONIC (not CLOCK_REALTIME).
     * CLOCK_MONOTONIC is unaffected by NTP adjustments or wall-clock jumps,
     * making it the correct choice for measuring elapsed execution time.
     */
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    engine_result_t result;
    int rc = run_engine(&config, &params, &result);

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    if (rc != 0) {
        fprintf(stderr, "Fatal: engine run failed.\n");
        return EXIT_FAILURE;
    }

    double elapsed = (double)(t_end.tv_sec  - t_start.tv_sec)
                   + (double)(t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    print_results(&config, &params, &result, elapsed);
    return EXIT_SUCCESS;
}
