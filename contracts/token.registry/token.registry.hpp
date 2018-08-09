/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP_5 Single Token Registry Interface
 * @author Craig Branscom
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>

using namespace eosio;
using namespace std;

class registry : public contract {

    public:
        // Constructor
        registry(account_name _self) : contract(_self),
            settings(_self, _self)
        {
            contractowner = _self;
        }

        account_name contractowner;

        // ABI Actions
        void init();

        void mint(account_name recipient, asset tokens);

        void transfer(account_name owner, account_name recipient, asset tokens);

        void allot(account_name owner, account_name recipient, asset tokens);

        void transferfrom(account_name owner, account_name recipient, asset tokens);

    protected:

        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name owner, asset tokens, account_name payer);
        
        //@abi table settings i64
        struct setting {
            account_name issuer;
            asset max_supply;
            asset supply;
            string name;
            bool is_initialized;

            uint64_t primary_key() const { return issuer; }
            EOSLIB_SERIALIZE(setting, (issuer)(max_supply)(supply)(name)(is_initialized))
        };
        
        //@abi table balances i64
        struct balance {
            account_name owner;
            asset tokens;

            uint64_t primary_key() const { return owner; }
            EOSLIB_SERIALIZE(balance, (owner)(tokens))
        };

        //@abi table allotments i64
        struct allotment {
            account_name owner;
            account_name recipient;
            asset tokens;

            uint64_t primary_key() const { return owner; }
            EOSLIB_SERIALIZE(allotment, (owner)(recipient)(tokens))
        };

        typedef multi_index< N(balances), balance> balances_table;
        typedef multi_index< N(allotments), allotment> allotments_table;

        typedef eosio::singleton<N(singleton), setting> settings_table;
        settings_table settings;
};