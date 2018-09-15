#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

struct votereceipt {
    uint64_t vote_code; // code of contract receiving vote //TODO: change to account_name type?
    uint64_t vote_scope; // scope of contract receiving vote
    uint64_t vote_key; // primary key to retrieve voted object
    uint16_t direction; // 0 = abstain, 1 = yes, 2 = no TODO: use enum? 
    uint64_t weight; // weight of votes applied
};

struct voterid { //TODO: separate tlos_weight into liquid and staked?
    account_name voter;
    vector<votereceipt> receipt_list; //list of all vote receipts
    uint64_t tlos_weight; //quantity of TLOS tokens, liquid and staked

    uint64_t primary_key() const { return voter; }
    EOSLIB_SERIALIZE(voterid, (voter)(votes_list)(tlos_weight))
};

struct environment {
    account_name publisher;

    uint64_t total_tokens;
    uint64_t total_voters;

    uint64_t primary_key() const { return publisher; }
    EOSLIB_SERIALIZE(environment, (publisher)(total_tokens)(total_voters))
};

typedef multi_index<N(voters), voterid> voters_table;
typedef singleton<N(environment), environment> environment_singleton;