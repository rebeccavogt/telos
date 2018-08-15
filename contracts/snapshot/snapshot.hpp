/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;

class snapshots : public contract {
  public:
     snapshots( account_name self )
     : contract(self),
     _snapshot_meta(_self, _self)
     {

     }

     void define( uint64_t snapshot_id,
                  uint64_t snapshot_block,
                  uint64_t expire_block );

     // TODO: add a setbalances method that can update multiple balances in a single action

     void setbalance( uint64_t snapshot_id,
                      account_name account,
                      uint64_t amount );

     void remove( block_id_type snapshot );

  private:
     //@abi table snapshots i64
     struct balance {
        account_name account;
        uint64_t amount;

        uint64_t primary_key()const { return account; }
     };

     //@abi table snapmeta i64
     struct snapshot {
         uint64_t snapshot_id;
         uint64_t snapshot_block;
         uint64_t expire_block;
         uint32_t last_updated;
         uint64_t max_supply;

         uint64_t primary_key()const { return snapshot_id; }
     };

     typedef eosio::multi_index<N(snapmeta), snapshot> SnapMeta;
     SnapMeta _snapshot_meta;

     typedef eosio::multi_index<N(snapshots), balance> SnapBalances;

};
