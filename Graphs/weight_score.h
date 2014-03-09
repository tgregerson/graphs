#ifndef WEIGHT_SCORE_H_
#define WEIGHT_SCORE_H_

#include <vector>

double ImbalancePower(const std::vector<int>& balance,
                      const std::vector<int>& max_weight_imbalance);

double NearViolaterImbalancePower(const std::vector<int>& balance,
                                  const std::vector<int>& max_weight_imbalance);

double RatioPower(const std::vector<int>& res_ratios,
                  const std::vector<int>& total_weight);

double RatioPowerIfChanged(
    const std::vector<int>& old_impl, const std::vector<int>& new_impl,
    const std::vector<int>& res_ratios,
    const std::vector<int>& total_weight);

#endif /* WEIGHT_SCORE_H_ */
