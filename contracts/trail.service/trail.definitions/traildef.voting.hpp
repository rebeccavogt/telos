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
 * Generic struct used to log a vote.
 * @field vote_code - code of contract where vote was cast
 * @field vote_scope - scope of table where vote was cast
 * @field vote_key - primary key of object that was voted on
 * @field vote_token - symbol_name of token used to vote
 * @field direction - direction of vote, where 0 = NO, 1 = YES, and 2 = ABSTAIN
 * @field weight - weight of vote, measured in token quantity
 * @field expiration - time_point of vote's expiration. Vote can be erased after expiring.
 * 
 * TODO: Could this be EOSLIB_SERIALIZED? stored in vector, not directly in table
 * TODO: reformat for removal of voterID struct, instead votereceipts are stored directly and indexed by 64 bit hash of code + scope + key
 */
struct votereceipt {
    uint64_t vote_code;
    uint64_t vote_scope;
    uint64_t vote_key;
    symbol_name vote_token;
    uint16_t direction;
    int64_t weight;
    uint32_t expiration;
};

/**
 * A VoterID card, used to track and store voter information and activity.
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
 * Environment struct tracking global eosio.trail information.
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
    EOSLIB_SERIALIZE(environment, (publisher)(total_tokens)(total_voters)(total_ballots))
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

bool is_voter(account_name voter) {
    voters_table voters(N(eosio.trail), voter); //TODO: change to _self, not good to hardcode account_name
    auto v = voters.find(voter);

    if (v != voters.end()) {
        return true;
    }

    return false;
}

bool has_receipt(account_name voter, uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key) {
    voters_table voters(N(eosio.trail), voter); //TODO: change to _self, not good to hardcode account_name
    auto vid = voters.get(voter);

    for (votereceipt vr : vid.receipt_list) {
        if (vr.vote_code == vote_code && vr.vote_scope == vote_scope && vr.vote_key == vote_key) {

            return true;
        }
    }

    return false;
}

voters_table::const_iterator find_receipt(account_name voter, uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key) {
    voters_table voters(N(eosio.trail), voter); //TODO: change to _self, don't hardcode account_name
    auto vid = voters.get(voter);

    auto itr = voters.begin();
    for (votereceipt vr : vid.receipt_list) {
        if (vr.vote_code == vote_code && vr.vote_scope == vote_scope && vr.vote_key == vote_key) {

            return itr;
        }
        itr++;
    }

    return voters.end();
}
