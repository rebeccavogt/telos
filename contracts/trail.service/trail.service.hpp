/**
 * 
 * 
 */

#include <trail.definitions/traildef.voting.hpp>
#include <trail.definitions/traildef.system.hpp>
#include <trail.definitions/traildef.tokens.hpp>

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
        /// @abi action
        void regtoken(asset native, account_name publisher);

        /**
         * Unregisters an existing token registry.
         * @param native - native asset of the token registry
         * @param publisher - account where the token registry is published
        */
        /// @abi action
        void unregtoken(asset native, account_name publisher);

        /**
         * Registers an account with a new VoterID.
         * @param voter - account for which to create a new VoterID
        */
        /// @abi action
        void regvoter(account_name voter);

        /**
         * Unregisters an account's existing VoterID.
         * @param voter - account owning VoterID to be unregistered
         * 
         * NOTE: All votereceipts must be removed first.
        */
        /// @abi action
        void unregvoter(account_name voter);

        /**
         * Adds a receipt to the receipt list on a VoterID.
         * @param vote_code - 
         * @param vote_scope - 
         * @param vote_key - 
         * @param vote_token - token symbol used to vote
         * @param direction - 
         * @param weight - 
         * @param expiration - 
         * @param voter - 
        */
        /// @action
        void addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, symbol_name vote_token, uint16_t direction, uint32_t expiration, account_name voter);

        /**
         * 
         */
        /// @abi action
        void rmvexpvotes(account_name voter);

        /**
         * Removes a receipt from the receipt list on a VoterID.
         * @param vote_code - 
         * @param vote_scope - 
         * @param vote_key - 
         * @param voter - 
        */
        //void rmvreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, account_name voter);

        /**
         * 
         */
        /// @abi action
        void regballot(account_name publisher);

        /**
         * 
         */
        /// @abi action
        void unregballot(account_name publisher);

    protected:

        environment_singleton env_singleton;
        environment env_struct;
};