#ifndef MODULES_DETECT_H
#define MODULES_DETECT_H

#include "block.h"
#include "util/detect.h"

namespace champsim::modules::detect
{
namespace branch_predictor
{
namespace detail
{
template <typename T>
using has_initialize = decltype(std::declval<T>().initialize_branch_predictor());

template <typename T>
using has_last_branch_result =
    decltype(std::declval<T>().last_branch_result(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<bool>(), std::declval<uint8_t>()));

template <typename T>
using has_predict_branch_v1 = decltype(std::declval<T>().predict_branch(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint64_t>(),
                                                                        std::declval<uint8_t>(), std::declval<uint8_t>()));

template <typename T>
using has_predict_branch_v2 = decltype(std::declval<T>().predict_branch(std::declval<uint64_t>()));
} // namespace detail

template <typename T>
constexpr auto has_initialize()
{
  return champsim::is_detected_v<detail::has_initialize, T>;
}

template <typename T>
constexpr auto has_last_branch_result()
{
  return champsim::is_detected_v<detail::has_last_branch_result, T>;
}

template <typename T>
constexpr auto has_predict_branch()
{
  if (champsim::is_detected_v<detail::has_predict_branch_v1, T>)
    return 1;
  if (champsim::is_detected_v<detail::has_predict_branch_v2, T>)
    return 2;
  return 0;
}
} // namespace branch_predictor

namespace btb
{
namespace detail
{
template <typename T>
using has_initialize = decltype(std::declval<T>().initialize_btb());

template <typename T>
using has_update_btb =
    decltype(std::declval<T>().update_btb(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<bool>(), std::declval<uint8_t>()));

template <typename T>
using has_btb_prediction_v1 = decltype(std::declval<T>().btb_prediction(std::declval<uint64_t>(), std::declval<uint8_t>()));

template <typename T>
using has_btb_prediction_v2 = decltype(std::declval<T>().btb_prediction(std::declval<uint64_t>()));
} // namespace detail

template <typename T>
constexpr auto has_initialize()
{
  return champsim::is_detected_v<detail::has_initialize, T>;
}

template <typename T>
constexpr auto has_update_btb()
{
  return champsim::is_detected_v<detail::has_update_btb, T>;
}

template <typename T>
constexpr auto has_btb_prediction()
{
  if (champsim::is_detected_v<detail::has_btb_prediction_v1, T>)
    return 1;
  if (champsim::is_detected_v<detail::has_btb_prediction_v2, T>)
    return 2;
  return 0;
}
} // namespace btb

namespace prefetcher
{
namespace detail
{
template <typename T>
using has_initialize = decltype(std::declval<T>().prefetcher_initialize());

template <typename T>
using has_cache_operate_v1 = decltype(std::declval<T>().prefetcher_cache_operate(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint8_t>(),
                                                                                 std::declval<uint8_t>(), std::declval<uint32_t>()));

template <typename T>
using has_cache_operate_v2 = decltype(std::declval<T>().prefetcher_cache_operate(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint8_t>(),
                                                                                 std::declval<bool>(), std::declval<uint8_t>(), std::declval<uint32_t>()));

template <typename T>
using has_cache_operate_v3 = decltype(std::declval<T>().prefetcher_cache_operate(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint8_t>(),
                                                                                 std::declval<bool>(), std::declval<access_type>(), std::declval<uint32_t>()));

template <typename T>
using has_cache_fill = decltype(std::declval<T>().prefetcher_cache_fill(std::declval<uint64_t>(), std::declval<long>(), std::declval<long>(),
                                                                        std::declval<uint8_t>(), std::declval<uint64_t>(), std::declval<uint32_t>()));

template <typename T>
using has_cycle_operate = decltype(std::declval<T>().prefetcher_cycle_operate());

template <typename T>
using has_final_stats = decltype(std::declval<T>().prefetcher_final_stats());

template <typename T>
using has_branch_operate = decltype(std::declval<T>().prefetcher_branch_operate(std::declval<uint64_t>(), std::declval<uint8_t>(), std::declval<uint64_t>()));
} // namespace detail

template <typename T>
constexpr bool has_initialize()
{
  return champsim::is_detected_v<detail::has_initialize, T>;
}

template <typename T>
constexpr auto has_cache_operate()
{
  if (champsim::is_detected_v<detail::has_cache_operate_v3, T>)
    return 3;
  if (champsim::is_detected_v<detail::has_cache_operate_v2, T>)
    return 2;
  if (champsim::is_detected_v<detail::has_cache_operate_v1, T>)
    return 1;
  return 0;
}

template <typename T>
constexpr auto has_cache_fill()
{
  return champsim::is_detected_v<detail::has_cache_fill, T>;
}

template <typename T>
constexpr bool has_cycle_operate()
{
  return champsim::is_detected_v<detail::has_cycle_operate, T>;
}

template <typename T>
constexpr bool has_final_stats()
{
  return champsim::is_detected_v<detail::has_final_stats, T>;
}

template <typename T>
constexpr bool has_branch_operate()
{
  return champsim::is_detected_v<detail::has_branch_operate, T>;
}
} // namespace prefetcher

namespace replacement
{
namespace detail
{
template <typename T>
using has_initialize = decltype(std::declval<T>().initialize_replacement());

template <typename T>
using has_find_victim_v1 =
    decltype(std::declval<T>().find_victim(std::declval<uint32_t>(), std::declval<uint64_t>(), std::declval<long>(), std::declval<champsim::cache_block*>(),
                                           std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint32_t>()));

template <typename T>
using has_find_victim_v2 =
    decltype(std::declval<T>().find_victim(std::declval<uint32_t>(), std::declval<uint64_t>(), std::declval<long>(), std::declval<champsim::cache_block*>(),
                                           std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<access_type>()));

template <typename T>
using has_update_state_v1 =
    decltype(std::declval<T>().update_replacement_state(std::declval<uint32_t>(), std::declval<long>(), std::declval<long>(), std::declval<uint64_t>(),
                                                        std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint32_t>(), std::declval<uint8_t>()));

template <typename T>
using has_update_state_v2 = decltype(std::declval<T>().update_replacement_state(std::declval<uint32_t>(), std::declval<long>(), std::declval<long>(),
                                                                                std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint64_t>(),
                                                                                std::declval<access_type>(), std::declval<uint8_t>()));

template <typename T>
using has_final_stats = decltype(std::declval<T>().replacement_final_stats());
} // namespace detail

template <typename T>
constexpr bool has_initialize()
{
  return champsim::is_detected_v<detail::has_initialize, T>;
}

template <typename T>
constexpr int has_find_victim()
{
  if (champsim::is_detected_v<detail::has_find_victim_v2, T>)
    return 2;
  if (champsim::is_detected_v<detail::has_find_victim_v1, T>)
    return 1;
  return 0;
}

template <typename T>
constexpr int has_update_state()
{
  if (champsim::is_detected_v<detail::has_update_state_v2, T>)
    return 2;
  if (champsim::is_detected_v<detail::has_update_state_v1, T>)
    return 1;
  return 0;
}

template <typename T>
constexpr bool has_final_stats()
{
  return champsim::is_detected_v<detail::has_final_stats, T>;
}
} // namespace replacement
} // namespace champsim::modules::detect

#endif
