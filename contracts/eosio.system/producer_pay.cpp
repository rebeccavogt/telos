#pragma once

#include "eosio.system.hpp"
#include <eosiolib/print.hpp>
#include <eosiolib/producer_schedule.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eosiolib/chain.h>
#include <algorithm>
#include <cmath>  

#define RESET_BLOCKS_PRODUCED 0
#define MAX_BLOCK_PER_CYCLE 12

namespace eosiosystem
{
using namespace eosio;

/**
* TELOS CHANGES:
*
* 1. Updated continuous_rate (inflation rate) to 2.5% --- DONE
* 2. Added producer_rate constant for BP/Standby payout --- DONE
* 3. Updated min_activated_stake to reflect TELOS 15% activation threshold --- DONE
* 4. Added worker_proposal_rate constant for Worker Proposal Fund --- DONE
* 5. Added min_unpaid_blocks_threshold constant for producer payout qualification --- DONE
*
* NOTE: A full breakdown of the calculated token supply and 15% activation threshold
* can be found at: https://docs.google.com/document/d/1K8w_Kd8Vmk_L0tAK56ETfAWlqgLo7r7Ae3JX25A5zVk/edit?usp=sharing
*/
const double continuous_rate = 0.025;                // 2.5% annual inflation rate
const uint64_t min_unpaid_blocks_threshold = 342;    // Minimum unpaid blocks required to qualify for Producer level payout

// Calculated constants
const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600; // half seconds per year
const uint32_t seconds_per_year = 52 * 7 * 24 * 3600;
const uint32_t blocks_per_day = 2 * 24 * 3600;
const uint32_t blocks_per_hour = 2 * 3600;
const uint64_t useconds_per_day = 24 * 3600 * uint64_t(1000000);
const uint64_t useconds_per_year = seconds_per_year * 1000000ll;

uint32_t active_schedule_size = 0;
uint32_t last_schedule_version = 0;
uint32_t cycle_counter = 0;

bool system_contract::crossed_missed_blocks_threshold(uint32_t amountBlocksMissed) {
    if(active_schedule_size <= 1) return false;

    //6hrs
    auto timeframe = (_grotations.next_rotation_time.to_time_point() - _grotations.last_rotation_time.to_time_point()).to_seconds();
   
    //get_active_producers returns the number of bytes populated
    // account_name prods[21];
    auto totalProds = active_schedule_size;//get_active_producers(prods, sizeof(account_name) * 21) / 8;
    //Total blocks that can be produced in a cycle
    auto maxBlocksPerCycle = (totalProds - 1) * MAX_BLOCK_PER_CYCLE;
    //total block that can be produced in the current timeframe
    auto totalBlocksPerTimeframe = (maxBlocksPerCycle * timeframe) / (maxBlocksPerCycle / 2);
    //max blocks that can be produced by a single producer in a timeframe
    auto maxBlocksPerProducer = (totalBlocksPerTimeframe * MAX_BLOCK_PER_CYCLE) / maxBlocksPerCycle;
    //15% is the max allowed missed blocks per single producer
    auto thresholdMissedBlocks = maxBlocksPerProducer * 0.15;
    
    return amountBlocksMissed > thresholdMissedBlocks;
}


void system_contract::reset_schedule_metrics() {
   for (auto &pm : _gschedule_metrics.producers_metric) { 
       if(pm.missed_blocks_per_cycle < 12) pm.missed_blocks_per_cycle = 12;
   }
}

void system_contract::reset_last_producer_missed_blocks() {
  // update last_onblock_caller
  for (auto &pm : _gschedule_metrics.producers_metric) {
    if (pm.missed_blocks_per_cycle > 0) {      
      auto pitr = _producers.find(pm.name);
      // update missed blocks per rotations
      if (pitr != _producers.end() && pitr->is_active) {
        _producers.modify(pitr, 0, [&](auto &p) {
          p.missed_blocks_per_rotation += pm.missed_blocks_per_cycle;
          //kick
        //   if(crossed_missed_blocks_threshold(p.missed_blocks_per_rotation)) {
        //       update_lifetime_metrics(p.owner, p.missed_blocks_per_rotation, p.unpaid_blocks);
        //       p.kick(kick_type::REACHED_TRESHOLD);
        //   }
        });
      }
    }
  }
}

void system_contract::producer_missed_few_blocks(account_name producer) {
  for (auto &pm : _gschedule_metrics.producers_metric) {
    if (pm.name == producer && pm.missed_blocks_per_cycle > 0) {

      auto pitr = _producers.find(pm.name);
      print("\n pm.is_active: ", pm.missed_blocks_per_cycle);
      if (pitr != _producers.end() && pitr->is_active) _gschedule_metrics.cycle_counter += pm.missed_blocks_per_cycle;
    }
  }
}

void system_contract::update_producer_missed_blocks(account_name producer) {
  for (auto &pm : _gschedule_metrics.producers_metric) {
    if (pm.name == producer && pm.missed_blocks_per_cycle > 0) {
      auto pitr = _producers.find(producer);
      if (pitr != _producers.end() && pitr->is_active){
        _gschedule_metrics.cycle_counter++;  
        pm.missed_blocks_per_cycle--;
      } 
      break;
    }
  }
}

bool system_contract::new_schedule_proposed(account_name active_schedule[], uint32_t size) {
    std::vector<account_name>new_schedule; 
    for(auto &p: _gschedule_metrics.producers_metric) new_schedule.emplace_back(p.name);

    std::sort(new_schedule.begin(), new_schedule.end());
    std::sort(active_schedule, active_schedule + size);

    for(size_t i = 0; i < _gstate.last_producer_schedule_size; i++){
        if(active_schedule[i] != new_schedule[i]) return true;
    }

    return false;
}

bool system_contract::check_missed_blocks(block_timestamp timestamp, account_name producer) {
  if (producer == N(eosio)) {
    print("\nprod: eosio");
    _gschedule_metrics.cycle_counter_correction++;
    return false;
  }
   
   bool error = false; 
   account_name producers_schedule[21];
   auto total_prods = get_active_producers(producers_schedule, sizeof(account_name) * 21) / 8; 
   
   if(_gstate.last_producer_schedule_size != total_prods) {
       print("\nschedule size diff");
       error = true;
   }
   
   //approx cycle time
   auto cycle_slots = total_prods * 12;
   bool is_new_schedule =  new_schedule_proposed(producers_schedule, total_prods);

   if(is_new_schedule && timestamp.slot - _gstate.last_proposed_schedule_update.slot < cycle_slots) {
       print("\nwaiting for new schedule to be active to start counting missed blocks again.");
       error = true;
   }    

   if(error) {
    print("\nerror found");   
    
    _gschedule_metrics.cycle_counter = 0; 
    _gschedule_metrics.last_onblock_caller = producer;
    return false;
   }

   if (_gschedule_metrics.cycle_counter_correction > 0) {
    print("\nblock needs to be corrected.");
    reset_schedule_metrics();

     for (auto &pm : _gschedule_metrics.producers_metric) {
       if (pm.name == producer) {
        auto pitr = _producers.find(producer);
        if (pitr != _producers.end() && pitr->is_active) {
            _gschedule_metrics.cycle_counter = _gschedule_metrics.cycle_counter_correction;
            pm.missed_blocks_per_cycle -= _gschedule_metrics.cycle_counter_correction;
        }
        break;
       }
     }

     _gschedule_metrics.cycle_counter_correction = 0;
     
     update_producer_missed_blocks(producer);
     _gschedule_metrics.last_onblock_caller = producer;  
     
     return false;
   } else {
     auto pitr = _producers.find(_gschedule_metrics.last_onblock_caller);
     if (pitr != _producers.end() && !pitr->is_active) {
       reset_schedule_metrics();
       print("\nlast_onblock_caller is deactivated.");
       print("\n_gschedule_metrics.cycle_counter: ", _gschedule_metrics.cycle_counter);
       
       update_producer_missed_blocks(producer);
       _gschedule_metrics.last_onblock_caller = producer;
       
       return false;
     }
   }

   auto pIdx = std::distance(producers_schedule, std::find(producers_schedule, producers_schedule + total_prods, producer));
   auto locIdx = std::distance(producers_schedule, std::find(producers_schedule, producers_schedule + total_prods, _gschedule_metrics.last_onblock_caller));
   auto diffIdx = std::abs(pIdx - locIdx);

   if (pIdx > locIdx && diffIdx > 1) {
     for (auto i = locIdx + 1; i < pIdx; i++) _gschedule_metrics.cycle_counter += 12;      
   } else if (pIdx < locIdx) {
     for (auto i = locIdx + 1; i < total_prods; i++) _gschedule_metrics.cycle_counter += 12; 

     for (auto i = 0; i < pIdx; i++) _gschedule_metrics.cycle_counter += 12;
     
   } 
   else if(_gschedule_metrics.last_onblock_caller != producer) {
     auto total_missed_blocks = 0;
     for (auto &pm : _gschedule_metrics.producers_metric) {
       total_missed_blocks += pm.missed_blocks_per_cycle;
     }

     if (_gschedule_metrics.cycle_counter != cycle_slots - total_missed_blocks) {
       producer_missed_few_blocks(_gschedule_metrics.last_onblock_caller);
     }
   }

   update_producer_missed_blocks(producer);
   
   _gschedule_metrics.last_onblock_caller = producer;
   
   print("\n_gschedule_metrics.cycle_counter: ", _gschedule_metrics.cycle_counter);
                     
   return _gschedule_metrics.cycle_counter >= cycle_slots;
}

void system_contract::onblock(block_timestamp timestamp, account_name producer) {
    require_auth(N(eosio));
    
    // Until activated stake crosses this threshold no new rewards are paid
    if (_gstate.thresh_activated_stake_time == 0) {
        _gstate.block_num++;
        
        if(_gstate.block_num >= block_num_network_activation && _gstate.total_producer_vote_weight > 0) _gstate.thresh_activated_stake_time = current_time();
        
        return;
    }
     
    if (_gstate.last_pervote_bucket_fill == 0) /// start the presses
        _gstate.last_pervote_bucket_fill = current_time();


    if(check_missed_blocks(timestamp, producer)) {
        reset_last_producer_missed_blocks();
        reset_schedule_metrics();
        _gschedule_metrics.cycle_counter = 0;
    }


    //check counter, reset missed blocks, reset counter
    /**
    * At startup the initial producer may not be one that is registered / elected
    * and therefore there may be no producer object for them.
    */
    auto prod = _producers.find(producer);
    if (prod != _producers.end()) {
        _gstate.total_unpaid_blocks++;
        _producers.modify(prod, 0, [&](auto &p) {
            p.unpaid_blocks++;
            p.lifetime_unpaid_blocks++;
        });
    }

    recalculate_votes();

    // Only update block producers once every minute, block_timestamp is in half seconds
    if (timestamp.slot - _gstate.last_producer_schedule_update.slot > 120) {
        
        update_elected_producers(timestamp);

        // Used in bidding for account names
        if ((timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day)
        {
            name_bid_table bids(_self, _self);
            auto idx = bids.get_index<N(highbid)>();
            auto highest = idx.begin();

            if (highest != idx.end() &&
                highest->high_bid > 0 &&
                highest->last_bid_time < (current_time() - useconds_per_day) &&
                _gstate.thresh_activated_stake_time > 0 &&
                (current_time() - _gstate.thresh_activated_stake_time) > 14 * useconds_per_day)
            {
                _gstate.last_name_close = timestamp;
                idx.modify(highest, 0, [&](auto &b) {
                    b.high_bid = -b.high_bid;
                });
            }
        }
    }

    //called once per day to set payments snapshot
    if (_gstate.last_claimrewards + uint32_t(172800) <= timestamp.slot) { //172800 blocks in a day
        print("\nNew ClaimRewards Snapshot");
        claimrewards_snapshot();
        _gstate.last_claimrewards = timestamp.slot;
    }
}

void system_contract::recalculate_votes(){
    // fix type 1 : fixes just total vote weight
    // iterate and fix the total_producer_vote_weight = _producers(sum) if < 0 
    // for the current issue on the testnet : this should be removed once the fix is applied
    // if (_gstate.total_producer_vote_weight <= -0.1){ // -0.1 threshold for floating point calc ?
    //     print("\n Negative total_weight_vote fix applied !");
    //     _gstate.total_producer_vote_weight = 0;
    //     for (const auto &prod : _producers) {
    //         _gstate.total_producer_vote_weight += prod.total_votes;
    //     }
    // }

    // fix type 2 : fixes proxied weights too
    if (_gstate.total_producer_vote_weight <= -0.1){ // -0.1 threshold for floating point calc ?
        _gstate.total_producer_vote_weight = 0;
        _gstate.total_activated_stake = 0;
        for(auto producer = _producers.begin(); producer != _producers.end(); ++producer){
            _producers.modify(producer, 0, [&](auto &p) {
                p.total_votes = 0;
            });
        }
        boost::container::flat_map<account_name, bool> processed_proxies;
        for (auto voter = _voters.begin(); voter != _voters.end(); ++voter) {
            if(voter->proxy && !processed_proxies[voter->proxy]){
                auto proxy = _voters.find(voter->proxy);
                _voters.modify( proxy, 0, [&]( auto& av ) {
                    av.last_vote_weight = 0;
                    av.last_stake = 0;
                    av.proxied_vote_weight = 0;
                });
                processed_proxies[voter->proxy] = true;
            }
            if(!voter->is_proxy || !processed_proxies[voter->owner]){
                _voters.modify( voter, 0, [&]( auto& av ) {
                    av.last_vote_weight = 0;
                    av.last_stake = 0;
                    av.proxied_vote_weight = 0;
                });
                processed_proxies[voter->owner] = true;
            }
            update_votes(voter->owner, voter->proxy, voter->producers, true);
        }
    }
}

void system_contract::claimrewards_snapshot(){
    require_auth(N(eosio)); //can only come from bp's onblock call

    eosio_assert(_gstate.thresh_activated_stake_time > 0, "cannot take snapshot until chain is activated");

    if (_gstate.total_unpaid_blocks <= 0) { //skips action, since there are no rewards to claim
        return;
    }

    auto ct = current_time();

    const asset token_supply = token(N(eosio.token)).get_supply(symbol_type(system_token_symbol).name());
    const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

    if (usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0)
    {
        auto new_tokens = static_cast<int64_t>((continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year));

        auto to_producers = (new_tokens / 5) * 2; //40% to producers
        auto to_workers = new_tokens - to_producers; //60% to WP's

        INLINE_ACTION_SENDER(eosio::token, issue)
        (N(eosio.token), {{N(eosio), N(active)}}, {N(eosio), asset(new_tokens), "Issue new TLOS tokens"});

        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {N(eosio), N(active)}, {N(eosio), N(eosio.saving), asset(to_workers), "Transfer worker proposal share to eosio.saving account"});

        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {N(eosio), N(active)}, {N(eosio), N(eosio.bpay), asset(to_producers), "Transfer producer share to per-block bucket"});

        _gstate.perblock_bucket += to_producers;
        _gstate.last_pervote_bucket_fill = ct;
    }

    //sort producers table
    auto sortedprods = _producers.get_index<N(prototalvote)>();

    uint32_t sharecount = 0;

    //calculate shares, should be between 2 and 72 shares
    for (const auto &item : sortedprods)
    {
        if (item.active()) { //only count activated producers
            if (sharecount <= 42) {
                sharecount += 2; //top producers count as double shares
            } else if (sharecount >= 43 && sharecount < 72) {
                sharecount++;
            } else {
                break; //no need to count past 72 shares
            }
        }
    }

    auto shareValue = (_gstate.perblock_bucket / sharecount);
    int32_t index = 0;

    for (const auto &prod : sortedprods) {

        if (!prod.active()) { //skip inactive producers
            continue;
        }

        int64_t pay_amount = 0;
        index++;
        
        if (index <= 21 && prod.unpaid_blocks >= min_unpaid_blocks_threshold) {
            pay_amount = (shareValue * int64_t(2));
        } else if (index <= 21 && prod.unpaid_blocks <= min_unpaid_blocks_threshold) {
            pay_amount = shareValue;
        } else if (index >= 22 && index <= 51) {
            pay_amount = shareValue;
        } else {
            pay_amount = 0; //edge case where outside top 51
        }

        _gstate.perblock_bucket -= pay_amount;
        _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

        _producers.modify(prod, 0, [&](auto &p) {
            p.last_claim_time = ct;
            p.unpaid_blocks = 0;
        });

        auto itr = payments.find(prod.owner);
        
        if (itr == payments.end()) {
            payments.emplace(prod.owner, [&]( auto& a ) { //have eosio pay? no issues so far...
                a.bp = prod.owner;
                a.pay = asset(pay_amount);
            });
        } else { //adds new payment to existing payment
            payments.modify(itr, 0, [&]( auto& a ) {
                a.pay += asset(pay_amount);
            });
        }
    }
}

/**
 * RATIONALE: Minimum Unpaid Blocks Threshold
 *
 * In the Telos Payment Architecture, block reward payments are calculated on the fly at the time
 * of the call to claimrewards. When called, the claimrewards function determines which payment level
 * each producer qualifies for and saves their payment to the payments table. Payments are then doled
 * out over the course of a day to the intended recipient.
 *
 * In order to qualify for a Producer level payout, the caller must be in the top 21 producers AND have
 * at least 12 hours worth of block production as a producer.
 */
void system_contract::claimrewards(const account_name &owner) {
    require_auth(owner);

    auto p = payments.find(owner);
    eosio_assert(p != payments.end(), "No payment exists for account");
    auto prod_payment = *p;
    auto pay_amount = prod_payment.pay;

    INLINE_ACTION_SENDER(eosio::token, transfer)
    (N(eosio.token), {N(eosio.bpay), N(active)}, {N(eosio.bpay), owner, pay_amount, std::string("Automatic Producer/Standby Payment")});

    payments.erase(p);
}

} //namespace eosiosystem
