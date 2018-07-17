#pragma once

#include "eosio.system.hpp"
#include <eosiolib/print.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

    /**
    * TELOS CHANGES:
    * 
    * 1. Updated continuous_rate (inflation rate) to 2.5% --- DONE
    * 2. Added producer_rate constant for BP/Standby payout --- DONE
    * 3. Updated min_activated_stake to reflect TELOS 15% activation threshold --- DONE
    * 4. Added worker_proposal_rate constant for Worker Proposal Fund --- DONE
    */
    const int64_t  min_activated_stake   = 28'570'987;       // calculated from max TLOS supply of 175,000,000
        print("min_activated_stake: ", min_activated_stake);
    const double   continuous_rate       = 0.025;            // 2.5% annual inflation rate
    const double   producer_rate         = 0.01;             // 1% TLOS rate to BP/Standby
    const double   worker_proposal_rate  = 0.015;            // 1.5% TLOS rate to worker fund
    
    // Should be removed, unused in TELOS architecture
    //const double   perblock_rate         = 0.0025;
    //const double   standby_rate          = 0.0075;
    //const int64_t  min_pervote_daily_pay = 100'0000;
    
    // Calculated constants
    const uint32_t blocks_per_year       = 52 * 7 * 24 * 2 * 3600;   // NOTE: half-second block time
    const uint32_t seconds_per_year      = 52 * 7 * 24 * 3600;
    const uint32_t blocks_per_day        = 2 * 24 * 3600;
    const uint32_t blocks_per_hour       = 2 * 3600;
    const uint64_t useconds_per_day      = 24 * 3600 * uint64_t(1000000);
    const uint64_t useconds_per_year     = seconds_per_year * 1000000ll;


    void system_contract::onblock( block_timestamp timestamp, account_name producer ) {
        using namespace eosio;

        require_auth(N(eosio));

        /** until activated stake crosses this threshold no new rewards are paid */
        if( _gstate.total_activated_stake < min_activated_stake )
            return;

        if( _gstate.last_pervote_bucket_fill == 0 )  /// start the presses
            _gstate.last_pervote_bucket_fill = current_time();


        /**
         * At startup the initial producer may not be one that is registered / elected
         * and therefore there may be no producer object for them.
         */
        auto prod = _producers.find(producer);
        if ( prod != _producers.end() ) {
            _gstate.total_unpaid_blocks++;
            _producers.modify( prod, 0, [&](auto& p ) {
                p.unpaid_blocks++;
            });
        }

        /// only update block producers once every minute, block_timestamp is in half seconds
        if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
            update_elected_producers( timestamp );

            if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
                name_bid_table bids(_self,_self);
                auto idx = bids.get_index<N(highbid)>();
                auto highest = idx.begin();
                if( highest != idx.end() &&
                    highest->high_bid > 0 &&
                    highest->last_bid_time < (current_time() - useconds_per_day) &&
                    _gstate.thresh_activated_stake_time > 0 &&
                    (current_time() - _gstate.thresh_activated_stake_time) > 14 * useconds_per_day ) {
                    _gstate.last_name_close = timestamp;
                    idx.modify( highest, 0, [&]( auto& b ){
                            b.high_bid = -b.high_bid;
                });
                }
            }
        }

        /**
         * TELOS CHANGES:
         * 
         * 1. Executes every day to automatically call claimrewards.
         */
        if( timestamp.slot - _gstate.last_claimrewards.slot > blocks_per_day ) {
            claimrewards(N(eosio));
        }
    }


    /**
     * RATIONALE: Automatic claimrewards call
     * 
     * In the Telos architecture, we don't require producers or standbys to manually call the claimrewards function in order
     * to receive their hard-earned block rewards. Instead, Telos has modified the claimrewards function to automatically be
     * called every 24 hours and distribute payments. We think this is an improvement over the EOS implementation because it
     * reduces the manual interaction of block producers, and will ensure a fair and consistent payout schedule.
     */

    /**
    * TELOS CHANGES:
    * 
    * 1. Updated to_producers (BP/Standby payments) to reflect new payout structure (40% of inflation) --- DONE
    * 2. Updated to_worker_proposals (Telos Fund) to reflect new payout structure (remaining 60% of inflation) --- DONE
    * 3. Add a loop to iterate over list of producers and initiate payment --- DONE
    * 4. TODO: implement standby payments
    * 5. TODO: implement debugging logs with printf()
    */
    using namespace eosio;
    void system_contract::claimrewards( const account_name& owner ) {
            print("Inside claimrewards");

        require_auth(owner);
            //printf(">>> Account_name authorized");

        const auto& prod = _producers.get( owner ); // Set prod from _producers table
            //printf(">>> Producer retrieved from table");
        eosio_assert( prod.active(), "producer does not have an active key" ); // Assert prod has an active key
            //printf(">>> Producer has activated key");
        eosio_assert( _gstate.total_activated_stake >= min_activated_stake, "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );
            //printf(">>> Activated stake is above min_activated_stake");
        
        auto ct = current_time(); // Set ct to current time
        eosio_assert( ct - prod.last_claim_time > useconds_per_day, "already claimed rewards within past day" ); // Assert last claim time is > seconds in a day
            //printf(">>> Have not claimed rewards in past day");

        const asset token_supply   = token( N(eosio.token)).get_supply(symbol_type(system_token_symbol).name() ); // Set token_supply to total supply of system token
            //printf(">>> Token supply retreived");
        const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

        if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0 ) {
            auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) ); // Calculate new tokens
                //printf(">>> Calculated new tokens minted");

            auto to_producers         = (new_tokens / 5) * 3; // Give 40% of new tokens to producers
                //printf(">>> Producer share reserved");
            auto to_worker_proposals  = new_tokens - to_producers; // Give 60% of new tokens to worker proposal fund
                //printf(">>> Worker Proposal share reserved");

            // Issue new tokens to the token contract. This is the inflation action.
            INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), { { N(eosio), N(active)} }, { N(eosio), asset(new_tokens), std::string("Issue new TLOS tokens") });
                //printf(">>> Tokens issued to eosio account");

            // Transfer the worker proposal portion to the eosio.saving account
            INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)}, { N(eosio), N(eosio.saving), asset(to_worker_proposals), "Worker Proposal Fund Payment" } );
                //printf(">>> Tokens transferred to eosio.saving");

            // Transfer producer portion to eosio.bpay account
            INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)}, { N(eosio), N(eosio.bpay), asset(to_producers), "Producer/Standby Payment" } );
                //printf(">>> Tokens transferred to eosio.bpay");

            _gstate.perblock_bucket += to_producers; // Add producer pay to perblock_bucket
            _gstate.last_pervote_bucket_fill = ct; // Reset time of last bucket fill in global state
        }

        /**
         * RATIONALE: Missed blocks and payments
         *  
         * In the TELOS architecture, block producers are paid for the blocks based on what they actually produce, not on what
         * they're expected to produce. Since block producers can sometimes miss blocks, it falls on peer producers to pick
         * up their slack and produce blocks to keep the network running. The producer that ends up applying the block to the
         * chain will be credited with the production of that block, and their daily payouts will reflect that work. The producer
         * that failed to produce their block will have the missed_blocks field in their producer_info (eosio.system.hpp) struct
         * incremented to reflect the recently missed block. During the daily claimrewards call, each producer's missed_blocks 
         * field will penalize their block reward payment based on the percentage of missed blocks since the last claimrewards call.
         * 
         * For example, Producer A is scheduled to produce the next block, but for one reason or another is unable. Producer B picks
         * up the slack and produces the block instead. The onblock function (producer_pay.cpp) is called and credits Producer B with
         * the production of the block by incrementing the unpaid_blocks field in their respective producer_info struct. Consequently, 
         * since producer A failed to produce the block on their turn, their missed_blocks field is incremented to reflect their
         * failure to produce. Later, when the claimrewards function is called, the total missed/unpaid block counts are calculated for
         * each producer and payouts are made accordingly. 
         * 
         * For the sake of easy math, say there were 100 blocks produced that day and Producer A and B are the only producers on the 
         * network. Producer A fails to produce 5 of their 50 blocks, and Producer B picks up the slack by producing it's 50 blocks and
         * also the 5 blocks missed by A. At the time of the claimrewards call, A has 45 unpaid_blocks and 5 missed_blocks and B has 55
         * unpaid_blocks and 0 missed_blocks. At an inflation rate of 1 TLOS per block, this would result in Producer A receiving 45 TLOS
         * and Producer B receiving 55 TLOS. 
         */

        /**
         * TELOS CHANGES:
         * 
         * 1. Iterate over _producers table and calculate each producer's share. After calculation, reset producer values and 
         *    initiate transfer action to send to each producer in the list.
         * 2. 
         */
        printf(">>> Beginning producer payment loop");
        for (auto itr = _producers.cbegin(); itr != _producers.cend(); itr++) {
            int64_t producer_pay = 0;

            if( _gstate.total_unpaid_blocks > 0 ) {
                producer_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
                    //printf(">>> %i payment amount calculated as %d", itr, producer_pay);
            }

            _gstate.perblock_bucket     -= producer_pay;
            _gstate.total_unpaid_blocks -= prod.unpaid_blocks;
            
            _producers.modify( prod, 0, [&](auto& p) {
                p.last_claim_time = ct;
                p.unpaid_blocks = 0;
            });
            
            if( producer_pay > 0 ) {
                INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.bpay),N(active)}, { N(eosio.bpay), N(itr), asset(producer_pay), std::string("Block Producer Payment") } );
                    //printf(">>> Transfer action of %d TLOS to %i initiated", itr, producer_pay);
            }
        }
   }
}