#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>

// special registers that help us identify branches
#define REG_STACK_POINTER 6
#define REG_FLAGS 25
#define REG_INSTRUCTION_POINTER 26

// branch types
enum branch_type {
  NOT_BRANCH = 0,
  BRANCH_DIRECT_JUMP = 1,
  BRANCH_INDIRECT = 2,
  BRANCH_CONDITIONAL = 3,
  BRANCH_DIRECT_CALL = 4,
  BRANCH_INDIRECT_CALL = 5,
  BRANCH_RETURN = 6,
  BRANCH_OTHER = 7
};

struct input_instr {
  // instruction pointer or PC (Program Counter)
  uint64_t ip = 0;

  // branch info
  uint8_t is_branch = 0;
  uint8_t branch_taken = 0;

  uint8_t destination_registers[2] = {}; // output registers
  uint8_t source_registers[4] = {};      // input registers

  uint64_t destination_memory[2] = {}; // output memory
  uint64_t source_memory[4] = {};      // input memory
};

struct cloudsuite_instr {
  // instruction pointer or PC (Program Counter)
  uint64_t ip = 0;

  // branch info
  uint8_t is_branch = 0;
  uint8_t branch_taken = 0;

  uint8_t destination_registers[4] = {}; // output registers
  uint8_t source_registers[4] = {};      // input registers

  uint64_t destination_memory[4] = {}; // output memory
  uint64_t source_memory[4] = {};      // input memory

  uint8_t asid[2] = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};
};

struct ooo_model_instr {
  uint64_t instr_id = 0;
  uint64_t ip = 0;
  uint64_t event_cycle = 0;

  bool is_branch = 0;
  bool branch_taken = 0;
  bool branch_prediction = 0;
  bool branch_mispredicted = 0; // A branch can be mispredicted even if the direction prediction is correct when the predicted target is not correct

  uint16_t asid;

  uint8_t branch_type = NOT_BRANCH;
  uint64_t branch_target = 0;

  uint8_t fetched = 0;
  uint8_t decoded = 0;
  uint8_t scheduled = 0;
  uint8_t executed = 0;

  int num_mem_ops = 0;
  int num_reg_dependent = 0;

  std::vector<uint8_t> destination_registers = {}; // output registers
  std::vector<uint8_t> source_registers = {};      // input registers

  std::vector<uint64_t> destination_memory = {};
  std::vector<uint64_t> source_memory = {};

  // these are indices of instructions in the ROB that depend on me
  std::vector<std::reference_wrapper<ooo_model_instr>> registers_instrs_depend_on_me;

private:
  template <typename T>
  ooo_model_instr(T instr, uint16_t asid) : ip(instr.ip), is_branch(instr.is_branch), branch_taken(instr.branch_taken), asid(asid)
  {
    std::remove_copy(std::begin(instr.destination_registers), std::end(instr.destination_registers), std::back_inserter(this->destination_registers), 0);
    std::remove_copy(std::begin(instr.source_registers), std::end(instr.source_registers), std::back_inserter(this->source_registers), 0);
    std::remove_copy(std::begin(instr.destination_memory), std::end(instr.destination_memory), std::back_inserter(this->destination_memory), 0);
    std::remove_copy(std::begin(instr.source_memory), std::end(instr.source_memory), std::back_inserter(this->source_memory), 0);
  }

public:
  explicit ooo_model_instr(input_instr instr) : ooo_model_instr(instr, std::numeric_limits<uint16_t>::max()) {}
  explicit ooo_model_instr(cloudsuite_instr instr) : ooo_model_instr(instr, (static_cast<uint16_t>(instr.asid[1]) << 8) + instr.asid[0]) {}
};

#endif
