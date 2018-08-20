/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP-5 Single Token Exchange Contract Extension
 * @author Craig Branscom
 * 
 * 
 */


#include "../token.registry.hpp"

class telex : public registry {
    public:

        struct registryinfo {
            asset native;
            account_name publisher;

            uint64_t primary_key() const { return native.symbol.name(); }
            uint64_t by_publisher() const { return publisher; }
        };

        typedef eosio::multi_index<N(registries), registryinfo> registries_table;

        /// @abi table orders
        struct order {
            uint64_t key;
            asset selling;
            asset buying;
            account_name maker;
            account_name fulfiller; //initialized to zero or seller account. will be reset during order match

            uint64_t primary_key() const { return key; }
            uint64_t by_buy_symbol() const { return buying.symbol.value; }
            uint64_t by_sell_symbol() const { return selling.symbol.value; }
            uint64_t by_maker() const { return maker; }
        };

        typedef multi_index< N(orders), order, 
            indexed_by< N(buying), const_mem_fun< order, uint64_t, &order::by_buy_symbol>>> orders_table;


        telex(account_name self) : registry(self) {
            
        }

        ~telex() {
            
        }

        /**
         * @brief Sell Native Token
         * 
         * NOTE: Always called to sell native asset
         */
        void sell(asset held, asset desired, account_name seller) {
            require_auth(seller);

                print("\nSell Action Called...");

            orders_table orders(_settings.issuer, seller);

            order s = order{
                orders.available_primary_key(),
                held,
                desired,
                seller,
                seller // NOTE: fulfiller field will be changed when order is bought
            };

            sub_balance(seller, held); // remove tokens from seller's balance

            attempt_sell(s);
        }

        /**
         * @brief Buy Native Token
         * 
         * NOTE: Always called to buy native asset
         */
        void buy(asset native, asset held, account_name buyer) {
            require_auth(buyer);

                print("\nBuy Action Called...");

            orders_table orders(_settings.issuer, buyer);
            
            order b = order{
                orders.available_primary_key(),
                native,
                held,
                buyer,
                buyer // NOTE: fulfiller field will be changed when order is sold
            };

            emplace_order(b);
        }

        void cancel() {

        }

    private:

    protected:

        inline void fulfill(order o) {
            
        }

        inline void get_contract(order o) {
            
        }

        /**
         * @brief Attempt to match a sell order with a buy order in target contract.
         */
        inline void attempt_sell(order o) {
            registries_table registries(N(eosio.token), o.buying.symbol.name()); // get registered table from master registry
            auto itr = registries.find(o.buying.symbol.name()); // find quote asset in master registry table

                print("\nSearching Master Registry for ", asset{o.buying} );

            if (itr == registries.end()) {
                print("\nSymbol not found in Master Registry...");
                emplace_order(o);
            } else {
                auto info = *itr;
                account_name target = info.publisher;
                // match order

                    print("\nTarget Contract: ", name{target});
            }
        }

        inline void attempt_buy(order o) {

        }

        /**
         * @brief Emplace an order into the orders table
         */
        inline void emplace_order(order o) {
            orders_table orders(_settings.issuer, o.maker);

            orders.emplace(o.maker, [&]( auto& a ){
                a.key = o.key;
                a.selling = o.selling;
                a.buying = o.buying;
                a.maker = o.maker;
                a.fulfiller = o.fulfiller;
            });

                print("\nOrder Emplaced...");
        }

        /**
        inline void emplace_buyorder(order o) {
            buyorders_table buyorders(_settings.issuer, _self);

            buyorders.emplace(o.maker, [&]( auto& a ){
                a.key = o.key;
                a.selling = o.selling;
                a.buying = o.buying;
                a.maker = o.maker;
                a.fulfiller = o.fulfiller;
            });

                print("\nBuy Order Emplaced...");
        }

        inline void emplace_sellorder(order o) {
            sellorders_table sellorders(_settings.issuer, _self);

            sellorders.emplace(o.maker, [&]( auto& a ){
                a.key = o.key;
                a.selling = o.selling;
                a.buying = o.buying;
                a.maker = o.maker;
                a.fulfiller = o.fulfiller;
            });

                print("\nSell Order Emplaced...");
        }
        */
};