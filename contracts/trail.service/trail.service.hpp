/**
 * 
 * 
 * @author Craig Branscom
 */

#include <trail.defs/trail.voting.hpp>
#include <trail.defs/trail.system.hpp>
#include <trail.defs/trail.tokens.hpp>

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <string>

using namespace eosio;

class trail : public contract {
    
    public:

        trail(account_name self);

        ~trail();

        #pragma region Token_Actions

        /// @abi action
        void regtoken(asset native, account_name publisher);

        /// @abi action
        void unregtoken(asset native, account_name publisher);

        #pragma endregion Token_Actions

        #pragma region Voting_Actions

        /// @abi action
        void regvoter(account_name voter);

        /// @abi action
        void unregvoter(account_name voter);

        /// @abi action
        void regballot(account_name publisher, asset voting_token, uint32_t begin_time, uint32_t end_time);

        /// @abi action
        void unregballot(account_name publisher, uint64_t ballot_id);

        /// @abi action
        void stakeforvote(account_name voter, asset amount);

        /// @abi action
        void unstakevotes(account_name voter, asset amount);

        /// @abi action
        void castvote(account_name voter, uint64_t ballot_id, asset amount, uint16_t direction);

        /// @abi action
        void closevote(account_name publisher, uint64_t ballot_id);

        #pragma endregion Voting_Actions

        #pragma region Reactions

        

        #pragma endregion Reactions

    protected:

        /// @abi table environment i64
        struct env {
            account_name publisher;
            
            uint64_t total_tokens;
            uint64_t total_voters;
            uint64_t total_ballots;

            uint32_t active_ballots;

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(env, (publisher)
                (total_tokens)(total_voters)(total_ballots)
                (active_ballots))
        };

        typedef singleton<N(environment), env> environment_singleton;

        environment_singleton environment;
        env env_struct;
};