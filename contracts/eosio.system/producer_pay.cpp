#pragma once

#include "eosio.system.hpp"
#include <eosiolib/print.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem
{

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
const int64_t min_activated_stake = 28'570'987'0000; // calculated from max TLOS supply of 190,473,249 (fluctuating value until mainnet activation)
const double continuous_rate = 0.025;                // 2.5% annual inflation rate
const double producer_rate = 0.01;                   // 1% TLOS rate to BP/Standby
const double worker_rate = 0.015;                    // 1.5% TLOS rate to worker fund
const uint64_t min_unpaid_blocks_threshold = 342;    // Minimum unpaid blocks required to qualify for Producer level payout

// Calculated constants
const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600; // half seconds per year
const uint32_t seconds_per_year = 52 * 7 * 24 * 3600;
const uint32_t blocks_per_day = 2 * 24 * 3600;
const uint32_t blocks_per_hour = 2 * 3600;
const uint64_t useconds_per_day = 24 * 3600 * uint64_t(1000000);
const uint64_t useconds_per_year = seconds_per_year * 1000000ll;

void system_contract::onblock(block_timestamp timestamp, account_name producer)
{
    using namespace eosio;

    require_auth(N(eosio));
    
    
    // Until activated stake crosses this threshold no new rewards are paid
    if (_gstate.total_activated_stake < min_activated_stake){
        print("\nonblock: network isn't activated");
        return;
    }
        
    // if (_gstate.last_pervote_bucket_fill == 0) /// start the presses
    //     _gstate.last_pervote_bucket_fill = current_time();

    /**
    * At startup the initial producer may not be one that is registered / elected
    * and therefore there may be no producer object for them.
    */
    auto prod = _producers.find(producer);
    if (prod != _producers.end())
    {
        _gstate.total_unpaid_blocks++;
        _producers.modify(prod, 0, [&](auto &p) {
            p.unpaid_blocks++;
        });
    }

    // Only update block producers once every minute, block_timestamp is in half seconds
    if (timestamp.slot - _gstate.last_producer_schedule_update.slot > 120)
    {
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
}

/**
    * TELOS CHANGES:
    *
    * 1. Updated to_producers (BP/Standby payments) to reflect new payout structure (40% of inflation) --- DONE
    * 2. Updated to_worker_proposals (Worker Proposal Fund) to reflect new payout structure (remaining 60% of inflation) --- DONE
    * 3. Implement debugging logs with print() --- DONE
    * 
    * TODO: Fix issue where the claimrewards debug output is shown twice
    */
using namespace eosio;
void system_contract::claimrewards(const account_name &owner)
{
    require_auth(owner);

    const auto &prod = _producers.get(owner);
    eosio_assert(prod.active(), "producer does not have an active key");

    eosio_assert(_gstate.total_activated_stake >= min_activated_stake,
                 "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)");

    auto ct = current_time();
    eosio_assert(ct - prod.last_claim_time > useconds_per_day, "already claimed rewards within past day");

    const asset token_supply = token(N(eosio.token)).get_supply(symbol_type(system_token_symbol).name());
    const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

    if (usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0)
    {
        auto new_tokens = static_cast<int64_t>((continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year));

        auto to_producers = (new_tokens / 5) * 2;
        auto to_workers = new_tokens - to_producers;

        INLINE_ACTION_SENDER(eosio::token, issue)
        (N(eosio.token), {{N(eosio), N(active)}}, {N(eosio), asset(new_tokens), "Issue new TLOS tokens"});

        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {N(eosio), N(active)}, {N(eosio), N(eosio.saving), asset(to_workers), "Transfer worker proposal share to eosio.saving account"});

        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {N(eosio), N(active)}, {N(eosio), N(eosio.bpay), asset(to_producers), "Transfer producer share to per-block bucket"});

        _gstate.perblock_bucket += to_producers;
        _gstate.last_pervote_bucket_fill = ct;

        //print("\nMinted Tokens: ", new_tokens);
        //print("\n   >Worker Fund: ", to_workers);
        //print("\n   >Producer Fund: ", to_producers);
    }

    //Sort _producers table
    auto sortedProds = _producers.get_index<N(prototalvote)>();

    uint32_t count = 0;
    uint32_t index = 0;

    // NOTE: Loop stops after 51st iteration. Counting farther than 51 is unnecessary when calculating payouts.
    for (const auto &item : sortedProds)
    {
        if (item.active()) { //Only count activated producers
            auto prodName = name{item.owner};
	        count++;

            if (owner == item.owner) {
                index = count;
                print("\nProducer Found: ", prodName);
                //print("\nIndex: ", index);
            }

            if (count >= 51) {
                break;
            }
        }
    }

    //print("\nTotal Count = ", count);

    uint32_t numProds = 0;
    uint32_t numStandbys = 0;
    int64_t totalShares = 0;

    // Calculate totalShares
    // TEST: Extensive testing to ensure no precision is lost during calculation of claimed rewards.
    if (count <= 21)
    {
        totalShares = (count * uint32_t(2));
        numProds = count;
        numStandbys = 0;
    } else {
        totalShares = (count + 21);
        numProds = 21;
        numStandbys = (count - 21);
    } 

    //print("\nnumProds: ", numProds);
    //print("\nnumStandbys: ", numStandbys);
    //print("\n_gstate.perblock_bucket: ", asset(_gstate.perblock_bucket));
    //print("\ntotalShares: ", totalShares);

    auto shareValue = (_gstate.perblock_bucket / totalShares);
    //print("\nshareValue: ", asset(shareValue));

    int64_t pay_amount = 0;

    /**
     * RATIONALE: Minimum Unpaid Blocks Threshold
     * 
     * In the Telos Payment Architecture, block reward payments are calculated on the fly at the time
     * of the call to claimrewards. When called, the claimrewards function determines which payment level
     * the calling account qualifies for and pays them accordingly.
     * 
     * In order to qualify for a Producer level payout, the caller must be in the top 21 producers AND have
     * at least 12 hours worth of block production as a producer. Requiring the calling account to have produced
     * a minimum of 12 hours worth of blocks ensures Standby's can't "jump the fence" just long enough to call 
     * claimrewards and take a producer's share of the payout.
     */

    // Determine if an account is a Producer or Standby, and calculate shares accordingly
    if (_gstate.total_unpaid_blocks > 0)
    {
        if (index <= 21 && prod.unpaid_blocks >= min_unpaid_blocks_threshold) {
            pay_amount = (shareValue * int64_t(2));
            print("\nCaller is a Producer @ ", index);
            print("\nUnpaid blocks: ", prod.unpaid_blocks);
            print("\nPayment: ", asset(pay_amount));
        } else if (index <= 21 && prod.unpaid_blocks <= min_unpaid_blocks_threshold) {
            pay_amount = shareValue;
            print("\nCaller is a Producer @ ", index);
            print("\nUnpaid blocks: ", prod.unpaid_blocks);
            print("\nCaller doesn't meet minimum unpaid blocks threshold of: ", min_unpaid_blocks_threshold);
            print("\nPayment: ", asset(pay_amount));
        } else if (index >= 22 && index <= 51) {
            pay_amount = shareValue;
            print("\nCaller is a Standby @ ", index);
            print("\nUnpaid blocks: ", prod.unpaid_blocks);
            print("\nPayment: ", asset(pay_amount));
        } else {
            pay_amount = 0;
            print("\nCaller is outside Top 51 @ ", index);
            print("\nUnpaid blocks: ", prod.unpaid_blocks);
            print("\nPayment: ", asset(pay_amount));
        }
    }

    /**
     * TODO: Implement Missed Block Deductions
     * 
     * Telos Payout Architecture will account for missed blocks, and reduce
     * payout based on percentage of missed blocks.
     * 
     * For instance, if Producer A misses 3 of their 12 blocks, they will
     * have missed 25% of their scheduled blocks. For easy math, say producers
     * earn 1 TLOS per block. This means Producer A will receive a payment of
     * 9 TLOS for their work, but the other 3 will remain in the bucket.
     */

    _gstate.perblock_bucket -= pay_amount;
    _gstate.total_unpaid_blocks -= prod.unpaid_blocks;
    
    _producers.modify(prod, 0, [&](auto &p) {
        p.last_claim_time = ct;
        p.unpaid_blocks = 0;
    });

    if (pay_amount > 0)
    {
        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {N(eosio.bpay), N(active)}, {N(eosio.bpay), owner, asset(pay_amount), std::string("Producer/Standby Payment")});
    }
}

} //namespace eosiosystem