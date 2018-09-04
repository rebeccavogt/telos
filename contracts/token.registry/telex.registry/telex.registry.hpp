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
#include <functional> //for hash function

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
            account_name taker; // initialized to zero, will be reset during order match
            uint64_t matched_key; // initialized to... 0? 

            uint64_t primary_key() const { return key; }
            uint64_t by_buy_symbol() const { return buying.symbol.value; }
            uint64_t by_sell_symbol() const { return selling.symbol.value; }
            uint64_t by_maker() const { return maker; }
            uint64_t by_taker() const { return taker; }
            uint64_t by_matched_key() const { return matched_key; }
        };

        typedef multi_index< N(orders), order, 
            indexed_by< N(buying), const_mem_fun< order, uint64_t, &order::by_buy_symbol>>> orders_table;
            // TODO: add selling index

        void marketorder(asset sell, asset buy, account_name maker) { //NOTE: sell amount doesn't matter for market orders
            require_auth(maker);
            eosio_assert(sell.symbol.name() == _settings.max_supply.symbol.name(), "selling asset must be native token");

            print("\nCreating market order...");


            orders_table orders(_settings.issuer, buy.symbol.name());

            order new_order = order{
                orders.available_primary_key(),
                sell,
                buy,
                maker,
                maker, // NOTE: taker field will be changed when order is sold...init to 0?
                0
            };

            sub_balance(maker, sell);

            auto t = find_target(new_order);

            if (t == 0) {
                print("\nCall regtoken from Registry to create new market...");
            } else {
                bool match = find_match(new_order, t);
                
                if (match) {
                    //inline action to target's fulfill action
                } else {
                    emplace_order(new_order); //emplaces unmatched order
                }
            }
        }

        void cancelorder(asset sell, asset buy, account_name maker) {
            require_auth(maker);

        }

        void collectorder(account_name taker, asset a, account_name target) { //change asset to symbol_type?
            require_auth(taker);
            eosio_assert(a.symbol.name() != _settings.max_supply.symbol.name(), "Cannot collect native token...call on target contract");

            orders_table orders(target, a.symbol.name()); //buying symbol as scope?
            auto itr = orders.find(a.symbol.name());
            auto o = *itr;

            if (itr == orders.end()) {
                print("\nNo Orders To Collect...");
            } else if (o.taker == taker) {
                add_balance(taker, o.selling, taker);
                orders.erase(itr);
                print("\nOrder Collected!");
            }
        }

        /**
         * Checks target contract for target key in orders(target, bought.symbol.name)
         * 
         * NOTE: fulfill is called from external contract. Make sure perspective is right...
         * NOTE: keep in mind self_key and target_key are switched for the calling contract
         */
        void fulfill(uint64_t self_key, uint64_t target_key, asset target_asset, account_name target) { //symbol_type instead of asset? or account_name if possible to do inline. saves computation

            order m = find_order(target, _settings.max_supply, target_key); // find order in other contract *NOT THIS CONTRACT* *TARGET_ASSET IS THIS CONTRACT'S NATIVE TOKEN*

            orders_table orders(_settings.issuer, target_asset.symbol.name()); //different scope? *THIS CONTRACT's ORDER*
            auto itr_2 = orders.find(self_key);
            eosio_assert(itr_2 != orders.end(), "No order found for self key");
            auto o = *itr_2;

            //order o1 = find_target();

            print("fulfill m.taker: ", name{m.taker});

            if (m.taker == o.maker) {
                orders.modify(itr_2, 0, [&]( auto& a ) {
                    a.taker = m.maker;
                });
            }

            // END OF TRX

            // NOTE: Now, the intended recipients are the only ones who can collect the orders

            // ALSO consider clearing out filled orders and turning into allotments. sitting orders can hinder performance since matching requires iterating over all orders in scope
        }

    private:



    protected:

        order find_order(account_name target_contract, asset target_asset, uint64_t target_key) {
            orders_table orders(target_contract, target_asset.symbol.name());
            auto itr = orders.find(target_key);
            eosio_assert(itr != orders.end(), "No order found for target key");
            auto m = *itr;
            return m;
        }

        account_name find_target(order o) {
            registries_table registries(N(prodname3s5x), o.buying.symbol.name()); // get registered table from master registry
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

        bool find_match(order o, account_name target) {
            orders_table orders(target, o.selling.symbol.name());

            int64_t i = 0;

            for (auto itr = orders.begin(); itr != orders.end();  ++itr ) {
                auto & m = *itr;

                i++;

                if (m.selling.symbol.name() == o.buying.symbol.name()) {
                    print("\nMatch Found...Order #", i);

                    print("\nFulfilling order...");

                    o.taker = m.maker;
                    emplace_order(o);

                    // hash OR just use key OR hash of key + match timestamp

                    // hash matched order...place hash in this order so target contract can validate

                    // make inline action send to target contract, telling them to check validity of this half of the fulfilled order, and to fulfill their half if validation passes
                    //fulfill(m.key, o.key, m.buying, target); //maybe o.selling? o.buying?

                    /*
                    INLINE_ACTION_SENDER(telex, fulfill)(target, 
                        {{N(eosio.code), N(active) }}, 
                        {m.key, o.key, m.buying, target}
                    );
                    */

                    //auto t = target;

                    //SEND_INLINE_ACTION( target, fulfill, {_settings.issuer, N(active)}, {m.key, o.key, m.buying, target} );
                    
                    /*
                    action{
                        permission_level{_self, N(active)},
                        target,
                        N(fulfill),
                        fulfill{
                            .origin_key=m.key, .target_key=o.key, .bought=m.buying, .target=target
                        }
                    }.send();
                    */

                    print("\nCalling target contract to process fulfillment...");
                    
                    //NOTE: switch self and target keys. self_key is target_key for action receiver, and vice versa
                    action(vector<permission_level>(), target, N(fulfill), make_tuple(
                        m.key, // self_key      
                        o.key, // target_key
                        o.selling, // target_asset
                        _settings.issuer // target
                    )).send();
                    
                    print("\nOrder Fulfilled by ", name{m.maker});
                    print("\n***Please collect your filled orders by calling collectorder()***");

                    return true;
                } else {
                    print("\nOrder #", i);
                }
            }

            print("\nMatch Not Found...");

            return false; // rethink return type...
        }

        void emplace_order(order o) {
            orders_table orders(_settings.issuer, o.buying.symbol.name()); //make buying symbol? selling symbol?

            orders.emplace(o.maker, [&]( auto& a ){
                a.key = orders.available_primary_key();
                a.selling = o.selling;
                a.buying = o.buying;
                a.maker = o.maker;
                a.taker = o.taker;
                a.matched_key = o.matched_key;
            });

            print("\nOrder Emplaced...");
        }

};