/**
 * This file includes all definitions necessary to interact with Trail's token registration system. Developers 
 * who want to utilize the system simply must include this file in their implementation to interact with the 
 * information stored by Trail.
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

/// @abi table registries i64
struct registration {
    asset native;
    account_name publisher;

    uint64_t primary_key() const { return native.symbol.name(); }
    uint64_t by_publisher() const { return publisher; }
    EOSLIB_SERIALIZE(registration, (native)(publisher))
};

struct balance {
    account_name owner;
    asset tokens;

    uint64_t primary_key() const { return owner; }
    EOSLIB_SERIALIZE(balance, (owner)(tokens))
};

typedef multi_index<N(balances), balance> balances_table;

typedef multi_index<N(registries), registration> registries_table;

bool is_trail_token(symbol_name sym) {
    registries_table registries(N(eosio.trail), sym);
    auto r = registries.find(sym);

    if (r != registries.end()) {
        return true;
    }

    return false;
}

registries_table::const_iterator find_registry(symbol_name sym) {
    registries_table registries(N(eosio.trail), sym);
    auto itr = registries.find(sym);

    if (itr != registries.end()) {
        return itr;
    }

    return registries.end();
}

registration get_registry(symbol_name sym) {
    registries_table registries(N(eosio.trail), sym);
    return registries.get(sym);
}

int64_t get_token_balance(symbol_name sym, account_name voter) {
    auto reg = get_registry(sym).publisher;

    balances_table balances(reg, voter);
    auto b = balances.get(voter);

    auto amount = b.tokens.amount;
    auto prec = b.tokens.symbol.precision();

    int64_t p10 = 1;
    while(prec > 0) {
        p10 *= 10; --prec;
    }

    return amount / p10;
}
