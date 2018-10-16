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

struct delegatebw_args {
    account_name from;
    account_name receiver;
    asset stake_net_quantity;
    asset stake_cpu_quantity;
    bool transfer;
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

    auto amount = acct.balance.amount;

    int64_t p10 = 1;
    while(prec > 0) {
        p10 *= 10; --prec;
    }

    return amount / p10;
}

asset get_liquid_tlos(account_name owner) {
    accounts accountstable(N(eosio.token), owner);
    auto a = accountstable.find(asset(0).symbol.name());

    int64_t amount = 0;

    if (a != accountstable.end()) {
        auto acct = *a;
        amount = acct.balance.amount;
    }
    
    return asset(amount);
}

asset get_staked_tlos(account_name owner) {
    user_resources_table userres(N(eosio), owner);
    auto r = userres.find(owner);

    int64_t amount = 0;

    if (r != userres.end()) {
        auto res = *r;
        amount = (res.cpu_weight.amount + res.net_weight.amount);
    }
    
    return asset(amount);
}

#pragma endregion Custom_Functions
