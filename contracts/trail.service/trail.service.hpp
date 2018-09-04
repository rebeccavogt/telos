/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace eosio;

class trail : public contract {
    public:
        trail( account_name self ):contract(self){}

        void regtoken(asset native, account_name publisher);

        void unregtoken(asset native, account_name publisher);

    private:

        /// @abi table registries i64
        struct registration {
            asset native;
            account_name publisher;

            uint64_t primary_key() const { return native.symbol.name(); }
            uint64_t by_publisher() const { return publisher; }
        };

        typedef eosio::multi_index<N(registries), registration> registries_table;

    protected:

};