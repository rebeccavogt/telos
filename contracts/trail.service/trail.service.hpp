/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>

using namespace eosio;

class trail : public contract {
    public:

        trail(account_name self);

        ~trail();

        void regtoken(asset native, account_name publisher);

        void unregtoken(asset native, account_name publisher);

        void regvoter(account_name voter);

        void unregvoter(account_name voter);

        void addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, uint16_t direction, uint64_t weight, account_name voter);

    protected:

        /// @abi table registries i64
        struct registration {
            asset native;
            account_name publisher;

            uint64_t primary_key() const { return native.symbol.name(); }
            uint64_t by_publisher() const { return publisher; }
        };

        struct voteinfo { //TODO: add checksum?
            uint64_t vote_code; // code of contract receiving vote
            uint64_t vote_scope; // scope of contract receiving vote
            uint64_t vote_key; // key to retrieve voted object in external contract
            uint16_t direction; // 0 = abstain, 1 = yes, 2 = no TODO: use enum? 
            uint64_t weight; // weight of votes applied
        };

        /// @abi table voters i64
        struct voterid {
            account_name voter;
            vector<voteinfo> votes_list;
            uint64_t tlos_weight;

            uint64_t primary_key() const { return voter; }
            EOSLIB_SERIALIZE(voterid, (voter)(votes_list)(tlos_weight))
        };

        /// @abi table environment
        struct environment {
            account_name publisher;

            uint64_t total_tokens;
            uint64_t total_voters;

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(environment, (publisher)(total_tokens)(total_voters))
        };

        typedef multi_index<N(registries), registration> registries_table;
        typedef multi_index<N(voters), voterid> voters_table;

        typedef singleton<N(environment), environment> environment_singleton;
        environment_singleton env;
        environment env_struct;
};