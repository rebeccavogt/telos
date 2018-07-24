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
    */
const int64_t min_activated_stake = 28'570'987; // calculated from max TLOS supply of 175,000,000
const double continuous_rate = 0.025;           // 2.5% annual inflation rate
const double producer_rate = 0.01;              // 1% TLOS rate to BP/Standby
const double worker_rate = 0.015;               // 1.5% TLOS rate to worker fund

// Constants should be removed, unused in TELOS architecture
//const double   perblock_rate         = 0.0025;         // 0.25%
//const double   standby_rate          = 0.0075;         // 0.75%

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
    if (_gstate.total_activated_stake < min_activated_stake)
        return;

    if (_gstate.last_pervote_bucket_fill == 0) /// start the presses
        _gstate.last_pervote_bucket_fill = current_time();

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
    * 2. Updated to_worker_proposals (Telos Fund) to reflect new payout structure (remaining 60% of inflation) --- DONE
    * 3. Implement debugging logs with print() --- DONE
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

    int64_t count = 0;
    int64_t index = 0;

    for (const auto &item : sortedProds)
    {
        //if (item.active) { // TODO: Only count activated producers
            auto prodName = name{item.owner};

            if (owner == item.owner) {
                index = count;
                print("\nProducer Found: ", prodName);
                //print("\nIndex: ", index);
            }

            count++;
        //}
    }

    //print("\nTotal Count = ", count);

    auto numProds = 0;
    auto numStandbys = 0;
    auto totalShares = 0;

    // Calculate totalShares
    // TODO: Check implicit conversion precision, off by 0.0005 TLOS
    if (count <= 21)
    {
        totalShares = (count * 2);
        numProds = count;
        numStandbys = 0;
    } else {
        totalShares = (count + 21);
        numProds = 21;
        numStandbys = (count - 21);
    } 

    //print("\nnumProds: ", numProds);
    //print("\nnumStandbys: ", numStandbys);
    //print("\n_gstate.perblock_bucket: ", _gstate.perblock_bucket);
    //print("\ntotalShares: ", totalShares);

    auto shareValue = (_gstate.perblock_bucket / totalShares);
    //print("\nshareValue: ", shareValue);

    auto pay_amount = 0;

    if (_gstate.total_unpaid_blocks > 0)
    {
        if (index <= 21) {
            pay_amount = (shareValue * 2);
            print("\nCaller is a Producer @ ", index);
        } else if (index >= 22 && index <= 51) {
            pay_amount = shareValue;
            print("\nCaller is a Standby @ ", index);
        } else {
            pay_amount = 0;
            print("\nCaller is outside Top 51 @ ", index);
        }
    }

    /**
     * TODO: Implement Missed Block Deductions
     * 
     * Telos Payout Architecture will account for missed blocks, and reduce
     * payout based on percentage of missed blocks.
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
        print("\nProducer/Standby Payment: ", asset(pay_amount));
    }
}

} //namespace eosiosystem
