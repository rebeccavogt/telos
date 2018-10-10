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

struct user_resources {
    account_name  owner;
    asset         net_weight;
    asset         cpu_weight;
    int64_t       ram_bytes = 0;

    uint64_t primary_key()const { return owner; }

    EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
};

typedef eosio::multi_index<N(accounts), account> accounts;
typedef eosio::multi_index<N(stat), currency_stats> stats;

typedef eosio::multi_index< N(userres), user_resources> user_resources_table;

#pragma region Custom_Functions

bool is_eosio_token(symbol_name sym, account_name owner) {
    accounts accountstable(N(eosio.token), owner);
    auto a = accountstable.find(sym);

    if (a != accountstable.end()) {
        return true;
    }

    return false;
}

int64_t get_eosio_token_balance(symbol_name sym, account_name owner) {
    accounts accountstable(N(eosio.token), owner);
    auto acct = accountstable.get(sym);
    auto prec = acct.balance.symbol.precision();

    return acct.balance.amount / (prec * 10); //NOTE: returns with precision applied
}

int64_t get_liquid_tlos(account_name owner) {
    accounts accountstable(N(eosio.token), owner);
    auto a = accountstable.find(asset(int64_t(0), S(4, TLOS)).symbol.name()); //TODO: find better way to get TLOS symbol?

    int64_t liquid_tlos = 0;

    if (a != accountstable.end()) {
        auto acct = *a;
        liquid_tlos = acct.balance.amount / int64_t(10000); //divide to get post-precision balance
    }
    
    return liquid_tlos;
}

int64_t get_staked_tlos(account_name owner) {
    user_resources_table userres(N(eosio), owner);
    auto r = userres.find(owner);

    int64_t staked_tlos = 0;

    if (r != userres.end()) {
        auto res = *r;
        staked_tlos = (res.cpu_weight.amount + res.net_weight.amount) / int64_t(10000); //divide to get post-precision balance
    }
    
    return staked_tlos;
}

//TODO: implement get_delegated_tlos()

#pragma endregion Custom_Functions
