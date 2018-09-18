#pragma once

#include <trail.connections/trailconn.voting.hpp> //Import trailservice voting data definitions

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <string>

using namespace eosio;

class trail : public contract {
    public:

        /**
         * Constructor sets environment singleton upon contract deployment, and gets environment for every action call.
        */
        trail(account_name self);

        /**
         * Destructor saves final state of environment struct at the end of every action call.
        */
        ~trail();

        /**
         * Registers a new token registry and its native asset.
         * @param native - native asset of token registry
         * @param publisher - account publishing token registry
        */
        void regtoken(asset native, account_name publisher);

        /**
         * Unregisters an existing token registry.
         * @param native - native asset of the token registry
         * @param publisher - account where the token registry is published
        */
        void unregtoken(asset native, account_name publisher);

        /**
         * Registers an account with a new VoterID.
         * @param voter - account for which to create a new VoterID
        */
        void regvoter(account_name voter);

        /**
         * Unregisters an account's existing VoterID.
         * @param voter - account owning VoterID to be unregistered 
        */
        void unregvoter(account_name voter);

        /**
         * Adds a receipt to the receipt list on a VoterID.
         * @param vote_code - 
         * @param vote_scope - 
         * @param vote_key - 
         * @param direction - 
         * @param weight - 
         * @param voter - 
        */
        void addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, uint16_t direction, account_name voter);

        /**
         * Removes a receipt from the receipt list on a VoterID.
         * @param vote_code - 
         * @param vote_scope - 
         * @param vote_key - 
         * @param voter - 
        */
        void rmvreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, account_name voter);

    protected:

        /**
         * 
        */
        /// @abi table registries i64
        struct registration {
            asset native;
            account_name publisher;

            uint64_t primary_key() const { return native.symbol.name(); }
            uint64_t by_publisher() const { return publisher; }
        };

        /*
        struct voteinfo { //TODO: change to votereceipt?
            uint64_t vote_code; // code of contract receiving vote //TODO: change to account_name?
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
        */

        typedef multi_index<N(registries), registration> registries_table;
        //typedef multi_index<N(voters), voterid> voters_table;

        //typedef singleton<N(environment), environment> environment_singleton;
        environment_singleton env_singleton;
        environment env_struct;
};