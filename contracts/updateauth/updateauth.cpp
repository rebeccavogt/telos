#include <eosiolib/eosio.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>

using namespace eosio;
using namespace std;

class updateauth : public eosio::contract {
  public:
      using contract::contract;

      /// @abi action 
      void update() {
		  require_auth(_self);
		  vector<key_weight> key_weights;
		  vector<permission_level_weight> permission_weights;
		  permission_weights.emplace_back(permission_level_weight{permission_level{ N(barkbarkbark), N(active) }, 1});

		  action(permission_level{ _self, N(owner) }, N(eosio), N(updateauth), make_tuple(
			  _self,
			  N(active),
			  N(owner),
			  authority{1, 0, key_weights, permission_weights}
		  )).send();
      }
  protected:

	struct permission_level_weight {
		permission_level  permission;
		weight_type       weight;
	};

	struct key_weight {
		public_key   key;
		weight_type  weight;
	};

	struct authority {
		uint32_t                              threshold;
		uint32_t                              delay_sec;
		std::vector<key_weight>               keys;
		std::vector<permission_level_weight>  accounts;
	};
};

EOSIO_ABI( updateauth, (update) )
