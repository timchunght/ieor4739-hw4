#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utilities.h"
#include "pnl.h"
#include "helpers.h"


int main(int argc, char *argv[])
{ 

  int num_of_assets;
  char *positions_filename = "./positions.txt";
  char *prices_filename = "./prices.txt";
  double max = 1e-10;
  double *x = NULL;
  int *indices = NULL;
  int t;
  int max_period = 10000;
  int runs_count = 1000000;
  double *prices = NULL;
  double budget = 1e10;
  double *deltas = NULL;
  double *sigmas = NULL;
  double *quantities = NULL;
  double *portfo_values = (double*)calloc(runs_count, sizeof(double));
  double *portfo_returns = (double*)calloc(runs_count, sizeof(double));
  
  int code = import_positions(positions_filename, &x, &num_of_assets, &indices, max);
  if(code != 0) {
    return code;
  }
  printf("positions:\n");
  printVector(num_of_assets, x);
  code = import_prices(prices_filename, &prices, num_of_assets, indices, &t, max_period);
  if(code != 0) {
    return code;
  }
  printf("prices:\n");
  printVector(num_of_assets, prices);


  
  // calculate array of averages of changes; array of Mu [Mui1, Mui2...]
  calculate_average_deltas(prices, num_of_assets, t, &deltas);
  printf("deltas:\n");
  printVector(num_of_assets, deltas);

  // calculate array of standard deviation of changes
  calculate_sigmas(prices, num_of_assets, t, deltas, &sigmas);
  printf("sigmas:\n");
  printVector(num_of_assets, sigmas);


  // calculate quantities
  calculate_quantities(num_of_assets, t, budget, x, prices, &quantities);
  printf("quantities:\n");
  printVector(num_of_assets, quantities);


  for(int z = 0; z < runs_count; z++) {
    code = run_simulation(z, num_of_assets, t, prices, quantities, deltas, sigmas, &portfo_values, &portfo_returns);
  }

  double avg_value = get_average(runs_count, portfo_values);
  double avg_return = get_average(runs_count, portfo_returns);

  printf("Average value: %g\n", avg_value);
  printf("Average return: %g\n", avg_return);

  free(portfo_values);
  free(portfo_returns);
}

int run_simulation(int run_idx, int num_of_assets, int t, double *prices, double *quantities, double *deltas, double *sigmas, double **portfo_values, double **portfo_returns) {

  int n = num_of_assets;
  double *p = prices;
  double *q = quantities;
  double portfo_value;
  double portfo_value_temp; 
  double portfo_return;
  unsigned int rseed = run_idx;

  portfo_return = 0.0;
  for (int j = 0; j < t; j++) {
    portfo_value = 0.0;
    for (int i = 0; i < n; i++) {
      portfo_value += (p[i*t + j] + (sigmas[i]*drawnormal_r(&rseed) + deltas[i])) * q[i];
    }
    if (j > 0) {
      portfo_return += (portfo_value - portfo_value_temp) / portfo_value_temp;
    }
    portfo_value_temp = portfo_value;
  }

  portfo_return = portfo_return/(t - 1);

  (*portfo_returns)[run_idx] = portfo_return;
  (*portfo_values)[run_idx] = portfo_value;
  printf("run %d: portfolio value: %g\n", run_idx, (*portfo_values)[run_idx]);
  printf("run %d: portfolio return: %g\n", run_idx, (*portfo_returns)[run_idx]);
  
  return 0;
}


