/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

class airgrab : public contract {
  public:
     airgrab( account_name self )
     : contract(self),
     _claims( _self, _self )
     {

     }

     void create( account_name issuer,
                  asset        maximum_supply);

     void issue( account_name to,
                 asset        quantity,
                 std::string  memo );

     void transfer( account_name from,
                    account_name to,
                    asset        quantity,
                    std::string  memo );


     void setclaimable( asset        claimable,
                        account_name snap_contract,
                        uint64_t     snap_id,
                        double       multiplier );

     void claim( account_name claimant, asset claimable );

     void clearclaim( account_name account, asset claimed );

     inline asset get_supply( symbol_name sym )const;

     inline asset get_balance( account_name owner, symbol_name sym )const;

  private:
     //@abi table accounts i64
     struct account {
        asset    balance;

        uint64_t primary_key()const { return balance.symbol.name(); }
     };

     //@abi table claims i64
     struct claimed_token {
         account_name account;
         asset        token;

         uint64_t primary_key()const { return account; }
     };

     //@abi table stat i64
     struct currencystat {
        asset          supply;
        asset          max_supply;
        account_name   issuer;
        bool           claimable;
        account_name   snap_contract;
        uint64_t       snap_id;
        double         multiplier;

        uint64_t primary_key()const { return supply.symbol.name(); }
     };

     typedef eosio::multi_index<N(accounts), account> accounts;

     typedef eosio::multi_index<N(stat), currencystat> stats;

     typedef eosio::multi_index<N(claims), claimed_token> claims;
     claims _claims;

     void sub_balance( account_name owner, asset value );
     void add_balance( account_name owner, asset value, account_name ram_payer );

     // Borrowed from snapshot.hpp
    struct balance {
        account_name account;
        uint64_t amount;

        uint64_t primary_key()const { return account; }
    };

    struct snapshot {
        uint64_t snapshot_id;
        uint64_t snapshot_block;
        uint64_t expire_block;
        uint32_t last_updated;
        uint64_t max_supply;

        uint64_t primary_key()const { return snapshot_id; }
    };

    typedef eosio::multi_index<N(snapmeta), snapshot> SnapMeta;
    typedef eosio::multi_index<N(snapshots), balance> SnapBalances;

  public:
     struct transfer_args {
        account_name  from;
        account_name  to;
        asset         quantity;
        std::string        memo;
     };
};

asset airgrab::get_supply( symbol_name sym )const
{
  stats statstable( _self, sym );
  const auto& st = statstable.get( sym );
  return st.supply;
}

asset airgrab::get_balance( account_name owner, symbol_name sym )const
{
  accounts accountstable( _self, owner );
  const auto& ac = accountstable.get( sym );
  return ac.balance;
}
