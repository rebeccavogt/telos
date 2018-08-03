/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

class sampletoken : public eosio::contract {

    public:
        sampletoken(account_name _self):contract( _self ){
            contract_owner = _self;
        }

        void mint(account_name recipient, asset tokens, string memo) {
            require_auth(contract_owner);
            //eosio_assert(tokens.amount + supply <= max_supply, "minting would exceed max_supply");
            add_balance(recipient, tokens, recipient);
        }

        void transfer(account_name owner, account_name recipient, asset tokens, string memo) {
            require_auth( owner );

            balances_table balances(contract_owner, contract_owner);
            const auto from = balances.get( owner, "owner account not found");
            eosio_assert(from.tokens >= tokens, "transaction would overdraw balance" );
            
            //.modify( fromacnt, from, [&]( auto& a ){ a.balance -= quantity; } );

            sub_balance( owner, tokens);
            add_balance( owner, tokens, recipient);
        }

        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name owner, asset tokens, account_name payer);

    private:
        

    protected:
        account_name contract_owner;

        int64_t max_supply;
        int64_t supply;
        symbol_type symbol;

        struct balance {
            account_name owner;
            asset tokens;

            //uint64_t primary_key()const { return holder; }
            EOSLIB_SERIALIZE( balance, ( owner )( tokens ) )
        };

        typedef multi_index< N(balances), balance> balances_table;
};

EOSIO_ABI( sampletoken, (mint)(transfer))