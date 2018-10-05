#include <eosiolib/eosio.hpp>
using namespace eosio;

class hello : public eosio::contract {
  public:
      using contract::contract;

      /// @abi action 
      void pushkey( public_key key ) { }
};

EOSIO_ABI( hello, (pushkey) )
