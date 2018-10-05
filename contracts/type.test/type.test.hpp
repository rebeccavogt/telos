#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class typecontract : public eosio::contract
{
  public:
	using contract::contract;

	typecontract(account_name self);

	~typecontract();

  protected:
	struct contract_types
	{
		account_name owner;
		asset amount;
		symbol_name symbol;
		public_key pub_key;

		string memo;
		
		uint16_t u_int_16;
		uint32_t u_int_32;
		uint64_t u_int_64;

		int16_t int_16;
		int32_t int_32;
		int64_t int_64;

		block_timestamp bts;
		time_point tp;
		time_point_sec tp_s;

		bool truth;

		vector<uint16_t> u_int_16s;
		vector<uint32_t> u_int_32s;
		vector<uint64_t> u_int_64s;

		vector<int16_t> int_16s;
		vector<int32_t> int_32s;
		vector<int64_t> int_64s;

		vector<asset> amounts;
		vector<account_name> owners;
		vector<string> memos;

		vector<bool> truths;

		vector<time_point_sec> v_tp_s;
		vector<time_point> v_tp;
		vector<block_timestamp> v_bts;
	};
};
