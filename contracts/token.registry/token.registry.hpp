/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP_5 Single Token Registry Interface
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

class registry : public contract {

    public:
        // Constructor
        registry(account_name _self):contract(_self){
            contractowner = _self;
            init();
        }

        account_name contractowner;

        // ABI Actions
        void init();

        void mint(account_name recipient, asset tokens, string memo);

        void transfer(account_name owner, account_name recipient, asset tokens, string memo);

        void allot(account_name owner, account_name recipient, asset tokens);

        void transferfrom(account_name owner, account_name recipient, asset tokens, string memo);

    protected:

        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name owner, asset tokens, account_name payer);
        
        //@abi table settings i64
        struct setting {
            string name;
            int64_t max_supply;
            int64_t supply;
            symbol_type symbol;
            bool is_initialized;

            uint64_t primary_key()const { return symbol.name(); }
            EOSLIB_SERIALIZE(setting, (name)(max_supply)(supply)(symbol)(is_initialized))
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
        typedef multi_index< N(settings), setting> settings_table;
};