#include "catch.hpp"
#include "vmem.h"

#include "dram_controller.h"

TEST_CASE("The virtual memory evaluates the correct shift amounts") {
  constexpr unsigned vmem_size_bits = 33;
  constexpr std::size_t log2_pte_page_size = 12;

  auto level = GENERATE(1,2,3,4,5);

  MEMORY_CONTROLLER dram{1, 3200, 12.5, 12.5, 12.5, 7.5};
  VirtualMemory uut{vmem_size_bits, 1 << log2_pte_page_size, 5, 200, dram};

  REQUIRE(uut.shamt(level) == LOG2_PAGE_SIZE + (log2_pte_page_size-champsim::lg2(PTE_BYTES))*(level-1));
}

TEST_CASE("The virtual memory evaluates the correct offsets") {
  constexpr unsigned vmem_size_bits = 33;
  constexpr std::size_t log2_pte_page_size = 12;

  auto level = GENERATE(as<unsigned>{}, 1,2,3,4,5);

  MEMORY_CONTROLLER dram{1, 3200, 12.5, 12.5, 12.5, 7.5};
  VirtualMemory uut{vmem_size_bits, 1 << log2_pte_page_size, 5, 200, dram};

  champsim::address addr{(0xffff'ffff'ffe0'0000 | (level << LOG2_PAGE_SIZE)) << ((level-1) * 9)};
  REQUIRE(uut.get_offset(addr, level) == level);
}
