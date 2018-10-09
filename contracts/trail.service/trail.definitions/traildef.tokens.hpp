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
};

typedef multi_index<N(registries), registration> registries_table;
