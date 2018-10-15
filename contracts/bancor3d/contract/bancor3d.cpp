#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <math.h>
#include "exchange_state.cpp"
#include <boost/algorithm/string.hpp>

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using eosio::string_to_name;
using std::string;
typedef long double real_type;

#define CORE_SYMBOL S(4,EOS)
#define TEAM_ACCOUNT N(ramisalive)

/**
 *
 *  Bancor3D.com - An Accelated Economic Game
 *
 *                      **   **
 *                 **************  *****
 *              **$$$$  ****   $$*******
 *            * $$$$$$$$$   $$* **   *$**
 *           *  $    $$$$$* $* *$$$$$$$$$**
 *          *  $$$$$$$$$$$$$ $$$$$$$$$**$$*
 *          $$  $$$$$$$ $$$$$$$$ $$$$$$$$$$$$
 *        *$ *$$$$$$$$$$$*$*$*$    *****$***
 *      **$$ $$$$$$*$ **    [[[[[[[   [[[
 *      **$**$$**$$$@@     [  #####  ## #
 *    *$$$*****$$$$$$         ..###   ###
 *    **$$$$$$$$ [[$$ ##        +++   ###
 *     $$$$$$**[[  [[$$    #          ##
 *       $$$***#[  #     ###      #   ##
 *       $$$$$$ ##++ #+        ## ######
 *       $$$$$#  ###       ##       ##
 *       $$$$$#   ##            ######
 *         $$$### ##      ##    #####
 *           $#$ #  +##  #   ##  ## #
 *           $$$$     #   +++#######
 *           $$*      ##############
 *           *$$# #  ++     ++++        
 *
 *  Bancor3D.com - An Accelated Economic Game
 *  Core Code as follows
 **/
class bancor3d : public eosio::contract {
  
  public:
    bancor3d(account_name self):
        eosio::contract(self), 
        _users(_self, _self), 
        _games(_self, _self),
        _rammarket(_self, _self){
    
        auto itr = _rammarket.find(S(4,RAMCORE));
        if( itr == _rammarket.end() ) {
            auto system_token_supply   = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name()).amount;
            if( system_token_supply > 0 ) {
                itr = _rammarket.emplace( _self, [&]( auto& m ) {
                m.supply.amount = 100000000000000ll;
                m.supply.symbol = S(4,RAMCORE);
                m.base.balance.amount = int64_t(1024000000); // start w/ 1GB of fake RAM
                m.base.balance.symbol = S(0,RAM);
                m.quote.balance.amount = system_token_supply / 1000;
                m.quote.balance.symbol = CORE_SYMBOL;
                });
            }
        } else {
            //print( "ram market already created" );
        }
    }

    void transfer( account_name from, account_name to, asset quant, std::string memo ) {
        require_auth( from );
        
        eosio_assert( now() >= INIT_TIME, "Game not started yet."); 
        if(quant.is_valid() && quant.symbol == S(4, EOS) && from != _self && to == _self)
        {
            if(quant.amount >= 10){
                eosio_assert( quant.amount >= 1000, "Minimum ticket to Keynes experiment is 0.1 EOS." );
                eosio_assert( quant.amount <= 10000*10000, "More than 10,000 EOS will make the designer run away." );
                eosio_assert( memo.size() <= 100 && memo.size() >=1, "Say something. Memo length should be less than 100 and greater than 1." );

                // calc game fee and update qty
                auto fee = quant;
                fee.amount = fee.amount * GAME_FEE_PERCENTAGE;

                auto quant_after_fee = quant;
                quant_after_fee.amount -= fee.amount;

                // calc in-game fees
                auto team_fee = fee * TEAM_FEE_PERCENTAGE;
                auto referral_fee = fee * REFERRAL_FEE_PERCENTAGE;
                auto daily_pot_fee = fee * DAILY_POT_FEE_PERCENTAGE;
                auto final_pot_fee = fee * FINAL_POT_FEE_PERCENTAGE;

                // send ram bytes
                int64_t bytes_out;
                const auto& market = _rammarket.get(S(4,RAMCORE), "ram market does not exist");
                _rammarket.modify( market, 0, [&]( auto& es ) {
                    bytes_out = es.convert( quant_after_fee,  S(0,RAM) ).amount;
                });
                eosio_assert( bytes_out > 0, "must reserve a positive amount" );

                // send pmt
                INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
                    { from, _self, quant, std::string("buy ram") } );

                std::string referral;

                if( fee.amount > 0 ) {
                    std::vector<std::string> referrals;
                    boost::split(referrals, memo, boost::is_any_of("-"));
                    if (referrals.size() > 1){
                        referral = referrals[1];
                        auto ref_itr = _users.find( referral );
                        if( ref_itr !=  _users.end() ) {
                            _users.modify( ref_itr, referral, [&]( auto& ref ) {
                                ref.referral_rewards += referral_fee.amount;
                            });
                            //INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)}, { _self, referral, referral_fee, std::string("referral fee") } );
                        } else { // add referral reward to pot
                            final_pot_fee.amount += referral_fee.amount;
                        }
                    } else { // add referral reward to pot
                        final_pot_fee.amount += referral_fee.amount;
                    }

                    // send team pmt
                    INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {to, N(active)},
                        { _self, TEAM_ACCOUNT, team_fee, std::string("team fee") } );
                }
                
                // udpate global game stats
                games.modify( gameitr, 0, [&]( auto& s ) {
                    s.ram_bytes += uint64_t(bytes_out);
                    s.final_pot += final_pot_fee.amount;
                    s.daily_pot += daily_pot_fee.amount;
                    s.team_pot += team_fee.amount;
                    s.referral_pot += referral_fee.amount;
                    s.eos_staked += quant_after_fee.amount;
                    s.last_referral = referral;
                    s.last_buyer = from;
                    s.last_update += get_time();
                });

                // update player game stats
                auto usr_itr = _users.find( from );
                if( usr_itr ==  _users.end() ) {
                    res_itr = _users.emplace( from, [&]( auto& usr ) {
                        usr.owner = from;
                    });
                }
                _users.modify( usr_itr, from, [&]( auto& usr ) {
                    usr.ram_bytes += bytes_out;
                    usr.eos_staked += quant_after_fee.amount;
                    usr.referrer = referral;
                    usr.last_update = get_time();
                });

                check_if_game_finalized();
            }
        }
    }
    

    //@abi action
    void sell(account_name from, asset quantity)
    {
        require_auth( from );

        eosio_assert( quantity.is_valid(), "Invalid RAM quantity.");
        eosio_assert( quantity.amount >= RAM_UNIT, "Invalid RAM amount - at least 1 RAM." );
        eosio_assert( quantity.amount <= 1000000 * RAM_UNIT, "Invalid RAM amount - at most 1M RAM." );

        auto useritr = users.find( from );
        eosio_assert( useritr != users.end(), "Unknown account from the RAM dictionary.");
        eosio_assert( useritr->k >= quantity.amount, "You don't have enough RAM.");




        eosio_assert( bytes > 0, "cannot sell negative byte" );

        user_resources_table  userres( _self, account );
        auto res_itr = userres.find( account );
        eosio_assert( res_itr != userres.end(), "no resource row" );
        eosio_assert( res_itr->ram_bytes >= bytes, "insufficient quota" );

        asset tokens_out;
        auto itr = _rammarket.find(S(4,RAMCORE));
        _rammarket.modify( itr, 0, [&]( auto& es ) {
            /// the cast to int64_t of bytes is safe because we certify bytes is <= quota which is limited by prior purchases
            tokens_out = es.convert( asset(bytes,S(0,RAM)), CORE_SYMBOL);
        });

        eosio_assert( tokens_out.amount > 1, "token amount received from selling ram is too low" );

        _gstate.total_ram_bytes_reserved -= static_cast<decltype(_gstate.total_ram_bytes_reserved)>(bytes); // bytes > 0 is asserted above
        _gstate.total_ram_stake          -= tokens_out.amount;

        //// this shouldn't happen, but just in case it does we should prevent it
        eosio_assert( _gstate.total_ram_stake >= 0, "error, attempt to unstake more tokens than previously staked" );

        userres.modify( res_itr, account, [&]( auto& res ) {
            res.ram_bytes -= bytes;
        });
        set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );

        INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.ram),N(active)},
                                                        { N(eosio.ram), account, asset(tokens_out), std::string("sell ram") } );

        auto fee = ( tokens_out.amount + 199 ) / 200; /// .5% fee (round up)
        // since tokens_out.amount was asserted to be at least 2 earlier, fee.amount < tokens_out.amount
        
        if( fee > 0 ) {
            INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {account,N(active)},
                { account, N(eosio.ramfee), asset(fee), std::string("sell ram fee") } );
        }







        double div = (double(useritr->e) - double(useritr->d)) * ratio;
        eosio_assert( ratio > 0 && ratio <= 1 , "Selling ratio should be (0, 1].");
        eosio_assert( useritr->e >= useritr->d && div >= 0 && useritr->e >= div , "Divident should be positive.");

        users.modify( useritr, 0, [&]( auto& s ) {
            s.k -= quantity.amount;
            s.p += eos;
            s.d += uint64_t(floor(div));
            if (s.k == 0) {
                s.t = 0;
            }
        });

        games.modify( gameitr, 0, [&]( auto& s ) {
            s.k -= quantity.amount;
            s.d += v;
            s.p -= v;
        });
    }

    uint64_t eos_days_hodl(uint64_t k, uint64_t t)
    {
        uint64_t keys = k / RAM_UNIT;
        uint64_t time_in_h = t / 3600 + 1;
        return keys * time_in_h;
    }

    void check_if_game_finalized()
    {
        
    }

    //@abi action
    void bigbang()
    {
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );

        uint64_t t_now = now();
        eosio_assert( gameitr->t < t_now, "Game has not finished yet." );
        eosio_assert( gameitr->p > 0, "Insufficient jackpot." );

        uint64_t user_payout = 0;
        double game_weight = 0;
        uint64_t user_payout;
        eosio_assert( gameitr->b >= user_payout, "Insufficient money for users." );
        user_get_bonus(user_payout);
    }

    //@abi action
    void jackpot()
    {
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );
        eosio_assert( gameitr->d >= 1000, "Insufficient jackpot dividends - minimum 0.1 EOS." );

        double game_weight = get_bonus(); // calcualte based on HODL Time

        eosio_assert( game_weight > 0, "No one deserves the jackpot dividends." );
        user_get_bonus(game_weight);

        games.modify( gameitr,0, [&]( auto& s ) {
            s.d = 0;
        });
    }

    //@abi action
    void withdraw( const account_name to) {
        require_auth( to );

        auto useritr = users.find( to );
        eosio_assert(useritr != users.end(), "Unknown account from Keynes' dictionary.");
        eosio_assert(useritr->p >= 1000, "Keynes does not like small change less than 0.1 EOS." );
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );

        asset balance(useritr->p,S(4,EOS));
        users.modify( useritr, 0, [&]( auto& s ) {
            s.p = 0;
        });

        action(
            permission_level{ _self, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(_self, to, balance, std::string("Good bye! - says Keynes. https://Bancor3D.com "))
        ).send();
    }

  private:
    const uint64_t RAM_UNIT = 10000;
    const uint64_t EOS_UNIT = 10000;
    const uint64_t TIME_INTERVAL = 3600 * 24 * 7; 
    const uint64_t INIT_TIME = 1538524800; // Wednesday, October 3, 2018 12:00:00 AM GMT

    const uint64_t GAME_FEE_PERCENTAGE = 0.25; // total fee 
    const uint64_t FINAL_POT_FEE_PERCENTAGE = 0.20; // % of fee sent to final pot
    const uint64_t DAILY_POT_FEE_PERCENTAGE = 0.05; // % of fee sent to daily pot
    const uint64_t REFERRAL_FEE_PERCENTAGE = 0.05; // % of fee sent to team
    const uint64_t TEAM_FEE_PERCENTAGE = 0.05; // % of fee sent to team

    // @abi table games i64
    struct game{
      uint64_t round;
      uint64_t ram_bytes;
      uint64_t final_pot;
      uint64_t daily_pot;
      uint64_t team_pot;
      uint64_t referral_pot;
      uint64_t eos_staked;
      uint64_t last_referral;
      uint64_t last_buyer;
      uint64_t last_update;
      uint64_t primary_key() const { return round; }
      EOSLIB_SERIALIZE(game, (round)(ram_bytes)(final_pot)(dialy_pot)(team_pot)(referral_pot)(eos_staked)(last_referral)(last_buyer)(last_update))
    };
    typedef eosio::multi_index<N(games), game> game_list;
    game_list _games;
    
    // @abi table users i64
    struct user {
      account_name owner;
      account_name referrer;
      uint64_t ram_bytes;
      uint64_t eos_staked;
      uint64_t referral_rewards;
      uint64_t last_update;
      uint64_t primary_key() const { return owner; }
      uint64_t get_key() const { return ram_bytes; }
      EOSLIB_SERIALIZE(user, (owner)(referrer)(ram_bytes)(eos_staked)(referral_rewards)(last_update))
    };
    typedef eosio::multi_index<N(users), user,
    indexed_by<N(k), const_mem_fun<user, uint64_t, &user::get_key>>
    > user_list;
    user_list _users;

    rammarket   _rammarket;
};

 #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
       if( action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if((code == N(eosio.token) && action == N(transfer)) || (code == self && (action==N(sell) || action == N(bigbang) || action == N(jackpot) || action == N(withdraw) || action == N(onerror))) ) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(bancor3d, (transfer)(sell)(bigbang)(jackpot)(withdraw))

