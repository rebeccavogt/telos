/**
 * IPFS Skeleton Contract
 * 
 * @author Craig Branscom
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

#include <eosiolib/print.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract]] ipfspay : public contract {
    
public:

    using contract::contract;

    const asset STORAGE_FEE = asset(10, symbol("TLOS", 4));

    ipfspay(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}



    struct [[eosio::table]] document {
        uint64_t storage_id;
        string ipfs_url;

        uint64_t primary_key() const { return storage_id; }
        EOSLIB_SERIALIZE(document, (storage_id)(ipfs_url))
    };

    typedef multi_index<name("storage"), document> storage_table;



    [[eosio::action]]
    void buystorage(name buyer) {
        require_auth(buyer);

        storage_table storage(_self, buyer.value);

        action(permission_level{ buyer, name("active") }, name("eosio.token"), name("transfer"), make_tuple(
            buyer,
            _self,
            STORAGE_FEE,
            std::string("Storage Fee")
        )).send();

        uint64_t new_id = storage.available_primary_key();

        storage.emplace(buyer, [&]( auto& a ){
            a.storage_id = new_id;
        });

        print("\nAssigned Storage ID: ", new_id);
    }

    [[eosio::action]]
    void savedoc(name owner, uint64_t storage_id, string ipfs_url){
        require_auth(owner);

        storage_table storage(_self, owner.value);
        auto s_itr = storage.find(storage_id);
        eosio_assert(s_itr != storage.end(), "invalid storage id");
        
        storage.modify(s_itr, same_payer, [&]( auto& a ) {
            a.ipfs_url = ipfs_url;
        });

        print("\nDocument Saved: ", ipfs_url);
    }

};

EOSIO_DISPATCH(ipfspay, (buystorage)(savedoc))