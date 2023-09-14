#include <catch.hpp>
#include "mocks.hpp"
#include "defaults.hpp"
#include "dram_controller.h"    
#include "champsim_constants.h"
#include <algorithm>
#include <cfenv>
#include <cmath>

std::vector<uint64_t> dram_test(MEMORY_CONTROLLER* uut, std::vector<champsim::channel::request_type>* packet_stream, std::vector<uint64_t>* arriv_time)
{
    uut->current_cycle = 0;

     auto ins_begin = std::begin(uut->channels[0].RQ);
    //load requests into controller
    std::transform(std::cbegin(*packet_stream), std::cend(*packet_stream), std::cbegin(*arriv_time), ins_begin, [](auto pkt, uint64_t cycle) {
        DRAM_CHANNEL::request_type r_pkt = DRAM_CHANNEL::request_type{pkt};
        r_pkt.forward_checked = false;
        r_pkt.event_cycle = cycle;
        return r_pkt;
    });

    //carry out operates, record request scheduling order
    std::vector<bool> last_scheduled(packet_stream->size(),false);
    std::vector<uint64_t> scheduled_order;

    while (std::size(scheduled_order) < std::size(*packet_stream))
    {
        //operate mem controller
        uut->_operate();
        //get scheduled requests
        std::vector<bool> next_scheduled{};
        std::transform(std::begin(uut->channels[0].RQ), std::end(uut->channels[0].RQ), std::back_inserter(next_scheduled), [](const auto& entry) { return ((entry.has_value() && entry.value().scheduled) || !entry.has_value()); });
        
        //search for newly scheduled requests
        auto chunk_begin = std::begin(next_scheduled);
        auto chunk_end = std::end(next_scheduled);
        while (chunk_begin != chunk_end) 
        {
            std::tie(chunk_begin, std::ignore) = std::mismatch(chunk_begin, chunk_end, std::cbegin(last_scheduled));
            //found newly scheduled request
            if (chunk_begin != chunk_end)
            {
               scheduled_order.push_back(std::distance(std::begin(next_scheduled), chunk_begin));
               break;
            }
        }
        //update vector of scheduled requests
        last_scheduled = next_scheduled;
    }
    return(scheduled_order);
}

SCENARIO("A series of reads arrive at the memory controller and are reordered") {
    GIVEN("A request stream to the memory controller") {
        MEMORY_CONTROLLER uut{1, 3200, 12.5, 12.5, 20, 7.5, {}};
        //test
        uut.warmup = false;
        uut.channels[0].warmup = false;
        //packets need address
        /*
        * | row address | rank index | column address | bank index | channel | block
        * offset |
        */
        std::vector<uint64_t> row_access = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
        std::vector<uint64_t> col_access = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
        std::vector<uint64_t> bak_access = {0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6};
        std::vector<uint64_t> arriv_time = {3,4,2,0,1,5,6,7,8,9,10,11,12,13,14,15,16,17,20,18,19};
        //we can expect the previous listed accesses to be reordered as such, as long as bank accesses are sufficiently lengthy
        //such that we can allocate requests to 6 additional banks before the first bank is done. The timing for the memory controller
        //is set within this test, so we can always expect this to be the case.
        std::vector<uint64_t> expected_order = {3,2,6,9,12,15,19,4,0,7,10,13,16,20,1,5,8,11,14,17,18};

        std::vector<champsim::channel::request_type> packet_stream;
        for(uint64_t i = 0; i < row_access.size(); i++)
        {
            auto pkt_type = access_type::LOAD;
            champsim::channel::request_type r;
            r.type = pkt_type;
            uint64_t offset = 0;
            champsim::address_slice<champsim::dynamic_extent> block_slice{LOG2_BLOCK_SIZE + offset, offset, 0};
            offset += LOG2_BLOCK_SIZE;
            champsim::address_slice<champsim::dynamic_extent> channel_slice{champsim::lg2(DRAM_CHANNELS) + offset, offset, 0};
            offset += champsim::lg2(DRAM_CHANNELS);
            champsim::address_slice<champsim::dynamic_extent> bank_slice{champsim::lg2(DRAM_BANKS) + offset, offset, bak_access[i]};
            offset += champsim::lg2(DRAM_BANKS);
            champsim::address_slice<champsim::dynamic_extent> column_slice{champsim::lg2(DRAM_COLUMNS) + offset, offset, col_access[i]};
            offset += champsim::lg2(DRAM_COLUMNS);
            champsim::address_slice<champsim::dynamic_extent> rank_slice{champsim::lg2(DRAM_RANKS) + offset, offset, 0};
            offset += champsim::lg2(DRAM_RANKS);
            champsim::address_slice<champsim::dynamic_extent> row_slice{64, offset, row_access[i]};
            r.address = champsim::address{champsim::splice(row_slice, rank_slice, column_slice, bank_slice, channel_slice, block_slice)};
            r.v_address = champsim::address{};
            r.instr_id = i;
            r.response_requested = false;
            packet_stream.push_back(r);
        }
        WHEN("The memory controller is operated") {
            std::vector<uint64_t> observed_order = dram_test(&uut,&packet_stream,&arriv_time);
            THEN("The memory controller scheduled packets according to the FR-FCFS scheme")
            {
                REQUIRE_THAT(expected_order, Catch::Matchers::RangeEquals(observed_order));
            }
        }
    }
}

