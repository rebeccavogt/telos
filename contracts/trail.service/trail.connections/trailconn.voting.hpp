#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

struct voteinfo {
    uint64_t vote_code; // code of contract receiving vote
    uint64_t vote_scope; // scope of contract receiving vote
    uint64_t vote_key; // key to retrieve voted object
    uint16_t direction; // 0 = abstain, 1 = yes, 2 = no TODO: use enum? 
    uint64_t weight; // weight of votes applied
};

struct voterid {
    account_name voter;
    vector<voteinfo> votes_list;
    uint64_t tlos_weight;

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