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

        telex(account_name self) : registry(self) {
            
        }

        ~telex() {
            
        }

        struct registration {
            asset native;
            account_name publisher;

            uint64_t primary_key() const { return native.symbol.name(); }
            uint64_t by_publisher() const { return publisher; }
        };

        typedef eosio::multi_index<N(registries), registration> registries_table;

        /// @abi table orders
        struct order {
            uint64_t key; //not used
            asset selling;
            asset buying;
            account_name maker;
            account_name taker; //initialized to zero, will be reset during order match

            uint64_t primary_key() const { return key; }
            uint64_t by_buy_symbol() const { return buying.symbol.value; }
            uint64_t by_sell_symbol() const { return selling.symbol.value; }
            uint64_t by_maker() const { return maker; }
        };

        typedef multi_index< N(orders), order, 
            indexed_by< N(buying), const_mem_fun< order, uint64_t, &order::by_buy_symbol>>> orders_table;
            // TODO: add selling index

        void marketorder(asset sell, asset buy, account_name maker) {
            require_auth(maker);
            eosio_assert(sell.symbol.name() == _settings.max_supply.symbol.name(), "selling asset must be native token");

            orders_table orders(_settings.issuer, buy.symbol.name());

            order m = order{
                orders.available_primary_key(),
                sell,
                buy,
                maker,
                maker // NOTE: fulfiller field will be changed when order is sold
            };

            auto t = find_target(m);

            if (t == 0) {
                print("\nCreating Market...");

                //emplace_order(m);
            } else {
                print("\nt: ", name{t});

                //attempt_fill(m);
            }
        }

        /**
         * @brief Sell Native Token
         * 
         * NOTE: Always called to sell native asset
         *
        void sell(asset held, asset desired, account_name seller) {
            require_auth(seller);
            eosio_assert(held.symbol.name() != desired.symbol.name(), "Buying and Selling asset cannot be same symbol");

            orders_table orders(_settings.issuer, seller);

            order s = order{
                orders.available_primary_key(),
                held,
                desired,
                seller,
                seller // NOTE: fulfiller field will be changed when order is bought
            };


        }

        
         * @brief Buy Native Token
         * 
         * NOTE: Always called to buy native asset
         *
        void buy(asset native, asset held, account_name buyer) {
            require_auth(buyer);
            eosio_assert(native.symbol.name() != held.symbol.name(), "Buying and Selling asset cannot be same symbol");

            orders_table orders(_settings.issuer, buyer);
            
            order b = order{
                orders.available_primary_key(),
                native,
                held,
                buyer,
                buyer // NOTE: fulfiller field will be changed when order is sold
            };

            if (find_target(b) == 0) { // not found in Master Registry
                emplace_order(b);
            } else {
                sub_balance(buyer, held);

            }
            

            //emplace_order(b);
        }
        */

        void cancelorder() {

        }

    private:

    protected:

        /**
         * @brief
         */
        account_name find_target(order o) {
            registries_table registries(N(eosio.token), o.buying.symbol.name()); // get registered table from master registry
            auto itr = registries.find(o.buying.symbol.name()); // find quote asset in master registry table

                print("\nSearching Master Registry for ", asset{o.buying} );

            if (itr == registries.end()) {
                print("\nSymbol not found in Master Registry...");
                return 0;
            } else {
                auto info = *itr;
                account_name target = info.publisher;
                print("\nSymbol found...Target Contract: ", name{target});
                return target;
            }
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

        inline void fulfill(order o) {
            
        }

        /**
         * @brief 
         */
        void attempt_fulfill(order o) {
            //auto t = find_target(o);


        }
};