/**
 * This contract defines the TIP-5 Single Token Interface.
 * 
 * For a full developer walkthrough go here: 
 * 
 * @author Craig Branscom
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>

using namespace eosio;
using namespace std;

class registry : public contract {

    public:
        registry(account_name self);

        ~registry();

        // ABI Actions
        void mint(account_name recipient, asset tokens);

        void transfer(account_name sender, account_name recipient, asset tokens);

        void allot(account_name sender, account_name recipient, asset tokens);

        void unallot(account_name sender, account_name recipient, asset tokens);

        void claimallot(account_name sender, account_name recipient, asset tokens);

        void createwallet(account_name recipient);

        void deletewallet(account_name owner);

    protected:

        //TODO: change to inlines?
        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name recipient, asset tokens, account_name payer);

        void sub_allot(account_name sender, account_name recipient, asset tokens);

        void add_allot(account_name sender, account_name recipient, asset tokens, account_name payer);
        
        //@abi table config i64
        struct tokenconfig {
            account_name publisher;
            string token_name;
            asset max_supply;
            asset supply;

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(tokenconfig, (publisher)(token_name)(max_supply)(supply))
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
            account_name recipient;
            account_name sender;
            asset tokens;

            uint64_t primary_key() const { return recipient; }
            uint64_t by_sender() const { return sender; }
            EOSLIB_SERIALIZE(allotment, (recipient)(sender)(tokens))
        };

        typedef multi_index< N(balances), balance> balances_table;

        typedef multi_index< N(allotments), allotment, 
            indexed_by<N(sender), const_mem_fun<allotment, uint64_t, &allotment::by_sender>>> allotments_table;

        typedef eosio::singleton<N(config), tokenconfig> config_singleton;
        config_singleton _config;
        tokenconfig config;
};