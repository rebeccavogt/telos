/**
 * This file includes all definitions necessary to interact with Trail's voting system. Developers who want to
 * utilize the system simply must include this file in their implementation to interact with the information
 * stored by Trail.
 * 
 * @author Craig Branscom
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

/**
 * @brief Generic struct used to log a vote.
 * @field vote_code - code of contract where vote was cast
 * @field vote_scope - scope of table where vote was cast
 * @field vote_key - primary key of object where vote is stored
 * @field direction - direction of vote, where 0 = NO, 1 = YES, and 2 = ABSTAIN
 * @field weight - weight of vote
 * @field expiration - time_point of vote's expiration. Vote can be erased after expiring.
 * 
 * TODO: Could this be EOSLIB_SERIALIZED?
 */
struct votereceipt {
    uint64_t vote_code; //TODO: change to account_name type?
    uint64_t vote_scope;
    uint64_t vote_key;
    uint16_t direction; //TODO: Use enum?
    int64_t weight;
    uint32_t expiration;
};

/**
 * @brief A VoterID card, used to track and store voter information and activity.
 * @field voter - account name of voter who owns the ID card
 * @field receipt_list - vector of all votereceipts
 */
/// @abi table voters i64
struct voterid {
    account_name voter;
    vector<votereceipt> receipt_list;

    uint64_t primary_key() const { return voter; }
    EOSLIB_SERIALIZE(voterid, (voter)(receipt_list))
};

/**
 * @brief Environment struct tracking global trailservice information.
 * @field publisher - account that published the contract.
 * @field total_tokens - total active number of unique tokens registered through the regtoken() action.
 * @field total_voters - total active number of unique voterid's registered through the regvoter() action.
 */
/// @abi table environment i64
struct environment {
    account_name publisher;
    uint64_t total_tokens;
    uint64_t total_voters;
    uint64_t total_ballots;

    uint64_t primary_key() const { return publisher; }
    EOSLIB_SERIALIZE(environment, (publisher)(total_tokens)(total_voters))
};

/// @abi table ballots
struct ballot {
    account_name publisher;

    uint64_t primary_key() const { return publisher; }
    EOSLIB_SERIALIZE(ballot, (publisher))
};

typedef multi_index<N(voters), voterid> voters_table;
typedef multi_index<N(ballots), ballot> ballots_table;
typedef singleton<N(environment), environment> environment_singleton;
