/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio.system.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <cmath>

#define VOTE_VARIATION 0.1
#define TWELVE_HOURS_US 43200000000
#define SIX_MINUTES_US 360000000

namespace eosiosystem {
   using namespace eosio;
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using eosio::singleton;
   using eosio::transaction;
   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of producer to register
    *
    */
   void system_contract::regproducer( const account_name producer, const eosio::public_key& producer_key, const std::string& url, uint16_t location ) {
      eosio_assert( url.size() < 512, "url too long" );
      eosio_assert( producer_key != eosio::public_key(), "public key should not be the default value" );
      require_auth( producer );

      auto prod = _producers.find( producer );

      if ( prod != _producers.end() ) {
         _producers.modify( prod, producer, [&]( producer_info& info ){
               info.producer_key = producer_key;
               info.is_active    = true;
               info.url          = url;
               info.location     = location;
            });
      } else {
         _producers.emplace( producer, [&]( producer_info& info ){
               info.owner         = producer;
               info.total_votes   = 0;
               info.producer_key  = producer_key;
               info.is_active     = true;
               info.url           = url;
               info.location      = location;
         });
      }
   }

   void system_contract::unregprod( const account_name producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer, "producer not found" );

      _producers.modify( prod, 0, [&]( producer_info& info ){
            info.deactivate();
      });
   }
  
   void system_contract::update_elected_producers( block_timestamp block_time ) {
      print("\ninit function");
      print("\nsbp_index: ", _grotations.sbp_in_index);
      print("\nbp_index: ", _grotations.bp_out_index);

      typedef std::pair<eosio::producer_key, uint16_t> producer_schedule_sig;
      _gstate.last_producer_schedule_update = block_time;

      auto idx = _producers.get_index<N(prototalvote)>();

      uint32_t distance = std::distance(idx.begin(), idx.end());
      distance = distance > 51 ? 51 : distance;
      
      print("\nTotal producers amount: ", distance);
      
      std::vector<producer_schedule_sig> producers;
      producers.reserve(distance);

      //add active producers with vote > 0
      for ( auto it = idx.cbegin(); it != idx.cend() && producers.size() < distance && it->total_votes > 0 && it->active(); ++it ) {
         producers.emplace_back( producer_schedule_sig({{it->owner, it->producer_key}, it->location}) );
      }

      distance = producers.size();
      
      vector<producer_schedule_sig>::iterator it_bp = producers.end();
      vector<producer_schedule_sig>::iterator it_sbp = producers.end();

      if (_grotations.next_rotation_time <= block_time)
      {
        print("\nRotation period expired");
        print("\nCurrent SBP Rotation: ", _grotations.sbp_currently_in);
        print("\nCurrent BP Rotation: ", _grotations.bp_currently_out);
        
        if (distance > 21) { //distance > 21 should rotate bps
          print("\nThere are more than 21 producers. calculating swaps...");
          _grotations.bp_out_index = _grotations.bp_out_index > 20 ? 0 : _grotations.bp_out_index + 1;
          _grotations.sbp_in_index = _grotations.sbp_in_index > distance ? 21 : _grotations.sbp_in_index + 1;
          
          print("\nsbp_index: ", _grotations.sbp_in_index);
          print("\nbp_index: ", _grotations.bp_out_index);

          account_name bp_name = producers[_grotations.bp_out_index].first.producer_name;
          account_name sbp_name = producers[_grotations.sbp_in_index].first.producer_name;
          
          print("\nBlock Producer to be rotated: ", name{bp_name});
          print("\nStandby Producer to be rotated: ",  name{sbp_name});

          //Find bp and sbt
          
          it_bp = std::find_if(producers.begin(), producers.end(), [&bp_name](const producer_schedule_sig &g) {
            return g.first.producer_name == bp_name; 
          });

          it_sbp = std::find_if(producers.begin(), producers.end(), [&sbp_name](const producer_schedule_sig &g) {
            return g.first.producer_name == sbp_name; 
          });

          //TODO: Check that 0 is an invalid account_name
          if (it_bp == producers.end() || it_sbp == producers.end())
          {
            print("\nBP or SPB don't exist. Setting bp in and sbp out to initial state.");
            _grotations.sbp_currently_in = 0;
            _grotations.bp_currently_out = 0;

            _grotations.bp_out_index = 21;
            _grotations.sbp_in_index = 75;
          }
          else
          {
            print("\nBP and SBP found. Setting bp in and sbp out to rotation table.");
            _grotations.sbp_currently_in = sbp_name;
            _grotations.bp_currently_out = bp_name;
          }
          _grotations.last_rotation_time = block_time;
          _grotations.next_rotation_time = block_timestamp(block_time.to_time_point() + time_point(microseconds(SIX_MINUTES_US)));
        } 
      } 
      else
      {
        //Check index initial state. bp index = 21 | sbp index = 75
        if(_grotations.bp_out_index < 21 && _grotations.sbp_currently_in < distance) {
          /*
          *Check if bp and sbt are still on the top 51.
          *If there is any change on the bp and sbp votes or they unregprod 
          *or the total list is less than 21
          *Rotations table should be updated to initial state.
          */

          account_name _bp = _grotations.bp_currently_out;
          it_bp = std::find_if(producers.begin(), producers.end(), [&_bp](const producer_schedule_sig &g) {
            return g.first.producer_name == _bp; 
          });

          account_name _sbp = _grotations.sbp_currently_in;
          it_sbp = std::find_if(producers.begin(), producers.end(), [&_sbp](const producer_schedule_sig &g) {
            return g.first.producer_name == _sbp; 
          });

          print("\nit_bp producer: ", name{it_bp->first.producer_name});
          //getting bp index to check if it still on top 21
          auto _bp_index = std::distance(producers.begin(), it_bp);
          
          print("\nit_sbp producer: ", name{it_sbp->first.producer_name});
          //getting sbp index to check if it still on 22 - 51
          auto _sbp_index = std::distance(producers.begin(), it_sbp);

          if(it_bp == producers.end() || it_sbp == producers.end() || distance < 21) {
            print("\nless than 21 producers and set global rotations table to inital state");            
            _grotations.bp_currently_out = 0;
            _grotations.sbp_currently_in = 0;

            _grotations.bp_out_index = 21;
            _grotations.sbp_in_index = 75;
          } else if (distance > 21 && (!is_in_range(_bp_index, 0, 21) || !is_in_range(_sbp_index, 21, 51))) {
            print("\nproducers list > 21 and bp or sbp out of range");
            _grotations.bp_currently_out = 0;
            _grotations.sbp_currently_in = 0;

            // _grotations.bp_out_index = 21;
            // _grotations.sbp_in_index = 75;
          }
        }
      }

      std::vector< producer_schedule_sig > top_producers;
      
      if(it_bp != producers.end() && it_sbp != producers.end()) {
        print("\nProducer removed: ", name{it_bp->first.producer_name});
        print("\nProducer added: ", name{it_sbp->first.producer_name});
        
        for ( auto pIt = producers.begin(); pIt != producers.end(); ++pIt) {
          auto i = std::distance(producers.begin(), pIt); 
          if(i > 20) break;

          if(pIt->first.producer_name == it_bp->first.producer_name) {
            top_producers.emplace_back(*it_sbp);
          } else {
            top_producers.emplace_back(*pIt);
          } 
       }
      } 
      else {
        top_producers = producers;
        top_producers.resize(21);
      }

      
      //doens't apply for telos
      // if ( top_producers.size() < _gstate.last_producer_schedule_size ) {
      //    return;
      // }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> new_producers;

      new_producers.reserve(top_producers.size());
      for( const auto& item : top_producers ){
         new_producers.push_back(item.first);
      }

      bytes packed_schedule = pack(new_producers);

      if( set_proposed_producers( packed_schedule.data(),  packed_schedule.size() ) >= 0 ) {
        print("\nschedule was proposed");

        for( const auto& item : top_producers ){
         print("\n*producer: ", name{item.first.producer_name});
        }

         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( new_producers.size() );
      }
   }
   
   /*
   * This function caculates the inverse weight voting. 
   * The maximum weighted vote will be reached if an account votes for the maximum number of registered producers (up to 30 in total).  
   */   
   double system_contract::inverseVoteWeight(double staked, double amountVotedProducers) {
     if (amountVotedProducers == 0.0) {
       return 0;
     }

     auto totalProducers = 0;
     for (const auto &prod : _producers) {
       if(prod.active()) { 
         totalProducers++;
       }
     }
     // 30 max producers allowed to vote
     if(totalProducers > 30) {
       totalProducers = 30;
     }

     double k = 1 - VOTE_VARIATION;
     
     return (k * sin(M_PI_2 * (amountVotedProducers / totalProducers)) + VOTE_VARIATION) * double(staked);
   }


   bool system_contract::is_in_range(int32_t index, int32_t low_bound, int32_t up_bound) {
     return index >= low_bound && index < up_bound;
   } 

   /**
    *  @pre producers must be sorted from lowest to highest and must be registered and active
    *  @pre if proxy is set then no producers can be voted for
    *  @pre if proxy is set then proxy account must exist and be registered as a proxy
    *  @pre every listed producer or proxy must have been previously registered
    *  @pre voter must authorize this action
    *  @pre voter must have previously staked some EOS for voting
    *  @pre voter->staked must be up to date
    *
    *  @post every producer previously voted for will have vote reduced by previous vote weight
    *  @post every producer newly voted for will have vote increased by new vote amount
    *  @post prior proxy will proxied_vote_weight decremented by previous vote weight
    *  @post new proxy will proxied_vote_weight incremented by new vote weight
    *
    *  If voting for a proxy, the producer votes will not change until the proxy updates their own vote.
    */
   void system_contract::voteproducer(const account_name voter_name, const account_name proxy, const std::vector<account_name> &producers) {
     require_auth(voter_name);
     update_votes(voter_name, proxy, producers, true);
   }
   
   void system_contract::checkNetworkActivation(){
     print("\nnetwork activated: ", _gstate.total_activated_stake >= min_activated_stake);
     print("\nnetwork activation time: ", _gstate.thresh_activated_stake_time);

     if( _gstate.total_activated_stake >= min_activated_stake && _gstate.thresh_activated_stake_time == 0 ) {
            _gstate.thresh_activated_stake_time = current_time();
    }
   } 

   void system_contract::update_votes( const account_name voter_name, const account_name proxy, const std::vector<account_name>& producers, bool voting ) {
      //validate input
      if ( proxy ) {
         eosio_assert( producers.size() == 0, "cannot vote for producers and proxy at same time" );
         eosio_assert( voter_name != proxy, "cannot proxy to self" );
         require_recipient( proxy );
      } else {
         eosio_assert( producers.size() <= 30, "attempt to vote for too many producers" );
         for( size_t i = 1; i < producers.size(); ++i ) {
            eosio_assert( producers[i-1] < producers[i], "producer votes must be unique and sorted" );
         }
      }

      auto voter = _voters.find(voter_name);
      eosio_assert( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object
      eosio_assert( !proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy" );

      auto totalStaked = voter->staked;
      if(proxy){
         auto pxy = _voters.find(proxy);
         totalStaked += pxy->proxied_vote_weight;
      }
      auto inverse_stake = inverseVoteWeight((double )totalStaked, (double) producers.size());
      auto new_vote_weight = inverse_stake;
      
      /**
       * The first time someone votes we calculate and set last_vote_weight, since they cannot unstake until
       * after total_activated_stake hits threshold, we can use last_vote_weight to determine that this is
       * their first vote and should consider their stake activated.
       * 
       * Setting a proxy will change the global staked if the proxied producer has voted. 
       */
      if( voter->last_vote_weight <= 0.0 && producers.size() > 0 && voting ) {
          _gstate.total_activated_stake += voter->staked;
          
          if(voter->proxied_vote_weight > 0) {
           _gstate.total_activated_stake += voter->proxied_vote_weight;
          }

          checkNetworkActivation();
      } else if(voter->last_vote_weight <= 0.0 && proxy && voting) {
        auto prx = _voters.find(proxy);
        if(prx->last_vote_weight > 0){
          _gstate.total_activated_stake += voter->staked;
          checkNetworkActivation();
        }
      } else if(producers.size() == 0 && !proxy && voting ) {
         _gstate.total_activated_stake -= voter->staked;

         if(voter->proxied_vote_weight > 0) {
           _gstate.total_activated_stake -= voter->proxied_vote_weight;
         }
      }

      boost::container::flat_map<account_name, pair<double, bool /*new*/> > producer_deltas;

      //Voter from second vote
      if ( voter->last_vote_weight > 0 ) {
           
         //if voter account has set proxy to a other voter account
         if( voter->proxy ) { 
            auto old_proxy = _voters.find( voter->proxy );

            eosio_assert( old_proxy != _voters.end(), "old proxy not found" ); //data corruption
            _voters.modify( old_proxy, 0, [&]( auto& vp ) {
                  vp.proxied_vote_weight -= voter->last_vote_weight;
               });
            propagate_weight_change( *old_proxy );
         } 
         else {
            for( const auto& p : voter->producers ) {
               auto& d = producer_deltas[p];
               d.first -= voter->last_vote_weight;
               d.second = false;
            }
         }
      }

      if( proxy ) {
         auto new_proxy = _voters.find( proxy );
         eosio_assert( new_proxy != _voters.end(), "invalid proxy specified" ); //if ( !voting ) { data corruption } else { wrong vote }
         eosio_assert( !voting || new_proxy->is_proxy, "proxy not found" );
        
        if( voting ) {
         _voters.modify( new_proxy, 0, [&]( auto& vp ) {
              vp.proxied_vote_weight += voter->staked;
            });
         } else {
            _voters.modify( new_proxy, 0, [&]( auto& vp ) {
              vp.proxied_vote_weight = voter->staked;
          });
        }
         
        if((*new_proxy).last_vote_weight > 0){
            propagate_weight_change( *new_proxy );
        }
      } else {
         if( new_vote_weight >= 0 ) {
           //if voter is proxied
           //remove staked provided to account and propagate new vote weight
            if(voter->proxy){
             auto old_proxy = _voters.find( voter->proxy );
              eosio_assert( old_proxy != _voters.end(), "old proxy not found" ); //data corruption
              _voters.modify( old_proxy, 0, [&]( auto& vp ) {
                  vp.proxied_vote_weight -= voter->staked;
               });
              propagate_weight_change( *old_proxy );
            }
            if( voting ) {
              for( const auto& p : producers ) {
                auto& d = producer_deltas[p]; 
                d.first += new_vote_weight;
                d.second = true;
              }
            } else { //delegate bandwidth
              if(voter->last_vote_weight > 0){
                propagate_weight_change(*voter);
              }
            }
         }
      }

      for( const auto& pd : producer_deltas ) {
         auto pitr = _producers.find( pd.first );
         if( pitr != _producers.end() ) {
            eosio_assert( !voting || pitr->active() || !pd.second.second /* not from new set */, "producer is not currently registered" );
            _producers.modify( pitr, 0, [&]( auto& p ) {
               p.total_votes += pd.second.first;
               if ( p.total_votes < 0 ) { // floating point arithmetics can give small negative numbers
                  p.total_votes = 0;
               }
               _gstate.total_producer_vote_weight += pd.second.first;
               //eosio_assert( p.total_votes >= 0, "something bad happened" );
            });
         } else {
            eosio_assert( !pd.second.second /* not from new set */, "producer is not registered" ); //data corruption
         }
      }

      _voters.modify( voter, 0, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.producers = producers;
         av.proxy     = proxy;
      });
   }

   /**
    *  An account marked as a proxy can vote with the weight of other accounts which
    *  have selected it as a proxy. Other accounts must refresh their voteproducer to
    *  update the proxy's weight.
    *
    *  @param isproxy - true if proxy wishes to vote on behalf of others, false otherwise
    *  @pre proxy must have something staked (existing row in voters table)
    *  @pre new state must be different than current state
    */
   void system_contract::regproxy( const account_name proxy, bool isproxy ) {
      require_auth( proxy );
      auto pitr = _voters.find(proxy);
      if ( pitr != _voters.end() ) {
         eosio_assert( isproxy != pitr->is_proxy, "action has no effect" );
         eosio_assert( !isproxy || !pitr->proxy, "account that uses a proxy is not allowed to become a proxy" );
         _voters.modify( pitr, 0, [&]( auto& p ) {
          p.is_proxy = isproxy;
        });    
      } else {
         _voters.emplace( proxy, [&]( auto& p ) {
            p.owner  = proxy;
            p.is_proxy = isproxy;
       });
      }
   }

   void system_contract::propagate_weight_change(const voter_info &voter) {
     eosio_assert( voter.proxy == 0 || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy");
     
     //getting all active producers
     auto totalProds = 0;
     for (const auto &prod : _producers) {
       if(prod.active()) { 
         totalProds++;
       }
     }
     auto totalStake = double(voter.staked) + voter.proxied_vote_weight;
     double new_weight = inverseVoteWeight(totalStake, totalProds);
    
     if (voter.proxy) {
       auto &proxy = _voters.get(voter.proxy, "proxy not found"); // data corruption
       _voters.modify(proxy, 0, [&](auto &p) { p.proxied_vote_weight = proxy.staked; });
       
       propagate_weight_change(proxy);
     } else {
       for (auto acnt : voter.producers) {
         auto &pitr = _producers.get(acnt, "producer not found"); // data corruption
         _producers.modify(pitr, 0, [&](auto &p) {
           p.total_votes = new_weight;
         });
       }
     }
     _voters.modify(voter, 0, [&](auto &v) { 
        v.last_vote_weight = new_weight; 
      });
   }

} /// namespace eosiosystem