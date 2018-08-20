/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP-5 Single Token Exchange Contract Extension
 * @author Craig Branscom
 */


#include <token.registry.hpp>

class telex : public registry {
    public:

        telex(account_name self) : registry(self) {
            print("Telex Constructor");
        }

        ~telex() {
            print("Telex Destructor");
        }

        // Selling Native Token
        void sell(asset held, asset desired, account_name seller) {
            require_auth(seller);
            
            orders_table orders(_settings.issuer, seller);

            auto itr = orders.find(seller);
            //attempt_sell();
        }

        // Buying Native Token
        void buy(asset native, asset held) {

        }

        

    private:

    protected:
        inline void emplace_sell() {
            
        }

        inline void attempt_sell(asset held, asset desired, account_name seller) {


        }

        inline void attemt_buy(asset native, asset held, account_name owner) {

        }

        /// @abi table orders i64
        struct order {
            account_name seller;
            asset selling;
            asset buying;

            uint64_t primary_key() const { return seller; }
            uint64_t by_symbol() const { return buying.symbol.value; }
        };

        typedef multi_index< N(orders), order, 
            indexed_by< N(buying), const_mem_fun<order, uint64_t, &order::by_symbol>>> orders_table;
        //orders_table orders;

        //typedef multi_index< N(fulfillments), order> fulfillments_table;
        //fulfillments_table fulfillments;


};

EOSIO_ABI( telex, (sell)(buy))