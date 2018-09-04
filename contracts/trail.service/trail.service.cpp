/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "trail.service.hpp"

/**
 * @brief Register A New Token with Master Registry
 * 
 * 
 */
void trail::regtoken(asset native, account_name publisher) {
    //require_auth(publisher);

    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto existing = registries.find(sym);

    if (existing == registries.end()) {
        registries.emplace(publisher, [&]( auto& a ){
            a.native = native;
            a.publisher = publisher;
        });

        print("\nEmplaced Registry...");
    } else {
        print("\nRegistry Already Exists...");
    }
}

/**
 * 
 */
void trail::unregtoken(asset native, account_name publisher) {
    //require_auth(publisher);
    
    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto existing = registries.find(sym);

    if (existing == registries.end()) {
        registries.erase(existing);

        print("\nEmplaced Registry...");
    } else {
        print("\nRegistry Already Exists...");
    }
}

EOSIO_ABI(trail, (regtoken)(unregtoken))
