/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP-5 Single Token Exchange Contract Extension
 * @author Craig Branscom
 */


#include "token.registry.hpp"

class telex : public registry {
    public:

        telex(account_name self) : registry(self), orders(self, self) {

        }

        ~telex();

        void sell(asset held, asset desired) {
            
        }

        void buy(asset native, asset held) {

        }

        

    private:

    protected:
        inline void attempt_sell(asset held, asset desired, account_name owner) {

        }

        inline void attemt_buy(asset native, asset held, account_name owner) {

        }

        struct order {
            asset selling;
            asset buying;
        };

        typedef multi_index< N(orders), order> orders_table;
        orders_table orders;

        //typedef multi_index< N(fulfillments), order> fulfillments_table;
        //fulfillments_table fulfillments;
};