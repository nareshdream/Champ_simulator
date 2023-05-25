/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "champsim.h"
#include "champsim_constants.h"
#include "core_inst.inc"
#include "phase_info.h"
#include "stats_printer.h"
#include "tracereader.h"
#include "vmem.h"

namespace champsim
{
std::vector<phase_stats> main(environment& env, std::vector<phase_info>& phases, std::vector<tracereader>& traces);
}

int main(int argc, char** argv)
{
  champsim::configured::generated_environment gen_environment{};

  CLI::App app{"A microarchitecture simulator for research and education"};

  bool knob_cloudsuite{false};
  uint64_t warmup_instructions = 0;
  uint64_t simulation_instructions = std::numeric_limits<uint64_t>::max();
  std::string json_file_name;
  std::vector<std::string> trace_names;

  auto set_heartbeat_callback = [&](auto){
    for (O3_CPU& cpu : gen_environment.cpu_view())
      cpu.show_heartbeat = false;
  };

  app.add_flag("-c,--cloudsuite", knob_cloudsuite, "Read all traces using the cloudsuite format");
  app.add_flag("--hide-heartbeat", set_heartbeat_callback, "Hide the heartbeat output");
  app.add_option("-w,--warmup-instructions", warmup_instructions, "The number of instructions in the warmup phase");
  auto sim_instr_option = app.add_option("-i,--simulation-instructions", simulation_instructions, "The number of instructions in the detailed phase. If not specified, run to the end of the trace.");

  auto json_option = app.add_option("--json", json_file_name, "The name of the file to receive JSON output. If no name is specified, stdout will be used")
    ->expected(0,1);

  app.add_option("traces", trace_names, "The paths to the traces")
    ->required()
    ->expected(NUM_CPUS)
    ->check(CLI::ExistingFile);

  CLI11_PARSE(app, argc, argv);

  std::vector<champsim::tracereader> traces;
  std::transform(std::begin(trace_names), std::end(trace_names), std::back_inserter(traces),
                 [knob_cloudsuite, repeat = (sim_instr_option->count > 0), i = uint8_t(0)](auto name) mutable {
                   return get_tracereader(name, i++, knob_cloudsuite, repeat);
                 });

  std::vector<champsim::phase_info> phases{
      {champsim::phase_info{"Warmup", true, warmup_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names},
       champsim::phase_info{"Simulation", false, simulation_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names}}};

  for (auto& p : phases)
    std::iota(std::begin(p.trace_index), std::end(p.trace_index), 0);

  std::cout << std::endl;
  std::cout << "*** ChampSim Multicore Out-of-Order Simulator ***" << std::endl;
  std::cout << std::endl;
  std::cout << "Warmup Instructions: " << phases[0].length << std::endl;
  std::cout << "Simulation Instructions: " << phases[1].length << std::endl;
  std::cout << "Number of CPUs: " << std::size(gen_environment.cpu_view()) << std::endl;
  std::cout << "Page size: " << PAGE_SIZE << std::endl;
  std::cout << std::endl;

  auto phase_stats = champsim::main(gen_environment, phases, traces);

  std::cout << std::endl;
  std::cout << "ChampSim completed all CPUs" << std::endl;
  std::cout << std::endl;

  champsim::plain_printer{std::cout}.print(phase_stats);

  for (CACHE& cache : gen_environment.cache_view())
    cache.impl_prefetcher_final_stats();

  for (CACHE& cache : gen_environment.cache_view())
    cache.impl_replacement_final_stats();

  if (json_option->count() > 0) {
    if (json_file_name.empty()) {
      champsim::json_printer{std::cout}.print(phase_stats);
    } else {
      std::ofstream json_file{json_file_name};
      champsim::json_printer{json_file}.print(phase_stats);
    }
  }

  return 0;
}
