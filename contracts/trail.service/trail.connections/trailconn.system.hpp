/**
 * This file includes all definitions necessary to interact with the system contract. Developers who want to
 * utilize this system simply must include this file in their implementation to interact with the information
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

struct account {
    asset    balance;

    uint64_t primary_key() const { return balance.symbol.name(); }
};

struct currency_stats {
    asset          supply;
    asset          max_supply;
    account_name   issuer;

    uint64_t primary_key() const { return supply.symbol.name(); }
};

struct transfer_args {
    account_name  from;
    account_name  to;
    asset         quantity;
    string        memo;
};

typedef eosio::multi_index<N(accounts), account> accounts;
typedef eosio::multi_index<N(stat), currency_stats> stats;

int64_t get_liquid_tlos(account_name voter) {
    accounts accountstable(N(eosio.token), voter);
    auto a = accountstable.find(asset(int64_t(0), S(4, TLOS)).symbol.name()); //TODO: find better way to get EOS symbol?

    int64_t liquid_tlos = 0;

    if (a != accountstable.end()) {
        auto acct = *a;
        liquid_tlos = acct.balance.amount / int64_t(10000); //divide to get actual balance
    }
    
    return liquid_tlos;
}
