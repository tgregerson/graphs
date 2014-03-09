#include "weight_score.h"

#include <cstdlib>

double ImbalancePower(const std::vector<int>& balance,
                      const std::vector<int>& max_weight_imbalance) {
  double imbalance_power = 0.0;
  for (int i = 0; i < balance.size(); i++) {
    int imb = max_weight_imbalance[i];
    if (imb == 0) {
      imb = 1;
    }
    double res_imbalance = abs((double)(balance[i]) / (double)imb);
    if (res_imbalance > 0.80) {
      res_imbalance *= 16;
    }
    imbalance_power += res_imbalance * res_imbalance;
  }
  return imbalance_power;
}

double NearViolaterImbalancePower(const std::vector<int>& balance,
    const std::vector<int>& max_weight_imbalance) {
  double imbalance_power = 0.0;
  for (int i = 0; i < balance.size(); i++) {
    int imb = max_weight_imbalance[i];
    if (imb == 0) {
      imb = 1;
    }
    double res_imbalance = abs((double)(balance[i]) / (double)imb);
    if (res_imbalance > 0.80) {
      imbalance_power += res_imbalance * res_imbalance;
    }
  }
  return imbalance_power;
}

double RatioPower(const std::vector<int>& res_ratios,
                  const std::vector<int>& total_weight) {
  // TODO Experiment with this.
  const double kSignificanceAdjustment = 10.0;
  int sum_total_weight = 0;
  int sum_ratio_weight = 0;
  int num_res = res_ratios.size();
  for (int i = 0; i < num_res; i++) {
    sum_total_weight += total_weight[i];
    sum_ratio_weight += res_ratios[i];
  }
  double scaler = (double)sum_total_weight / (double)sum_ratio_weight;
  std::vector<int> target_total_weights(num_res);
  double ratio_power = 0.0;
  for (int i = 0; i < num_res; i++) {
    double target_total_weight = res_ratios[i] * scaler;
    double imb = abs(total_weight[i] - target_total_weight) /
        target_total_weight;
    ratio_power += (imb * imb);
  }
  return ratio_power / kSignificanceAdjustment;
}

double RatioPowerIfChanged(
    const std::vector<int>& old_impl, const std::vector<int>& new_impl,
    const std::vector<int>& res_ratios,
    const std::vector<int>& total_weight) {
  int num_res = res_ratios.size();
  std::vector<int> new_total_weight(num_res);
  for (int i = 0; i < num_res; i++) {
    new_total_weight[i] = total_weight[i] + new_impl[i] - old_impl[i];
  }
  return RatioPower(res_ratios, new_total_weight);
}
