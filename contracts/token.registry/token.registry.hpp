/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

class registry : public contract {

    public:
        // Constructor
        registry(account_name _self):contract( _self ){
            contract_owner = _self;
            init();
        }

        // ABI Actions
        void mint(account_name recipient, asset tokens, string memo);

        void transfer(account_name owner, account_name recipient, asset tokens, string memo);

        void allot(account_name owner, account_name recipient, asset tokens);

        void transferfrom(account_name owner, account_name recipient, asset tokens, string memo);

    protected:
        account_name contract_owner;

        void init();

        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name owner, asset tokens, account_name payer);
        
        //@abi table info i64
        struct token_info {
            string name;
            int64_t max_supply;
            int64_t supply;
            symbol_type symbol;
            bool is_initialized;

            uint64_t primary_key()const { return symbol.name(); }
            EOSLIB_SERIALIZE(token_info, (name)(max_supply)(supply)(symbol)(is_initialized))
        };
        
        //@abi table balances i64
        struct balance {
            account_name owner;
            asset tokens;

            uint64_t primary_key()const { return owner; }
            EOSLIB_SERIALIZE(balance, (owner)(tokens))
        };

        //@abi table allotments i64
        struct allotment {
            account_name owner;
            account_name recipient;
            asset tokens;

            uint64_t primary_key()const { return owner; }
            EOSLIB_SERIALIZE(allotment, (owner)(recipient)(tokens))
        };

        typedef multi_index< N(balances), balance> balances_table;
        typedef multi_index< N(allotments), allotment> allotments_table;
        typedef multi_index< N(settings), token_info> settings_table;
};