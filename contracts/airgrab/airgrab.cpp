/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */

#include "airgrab.hpp"

void airgrab::create( account_name issuer,
                      asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void airgrab::issue( account_name to, asset quantity, std::string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void airgrab::transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        std::string  memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void airgrab::setclaimable( asset        claimable,
                             account_name snap_contract,
                             uint64_t     snap_id,
                             double       multiplier ) {
    auto sym = claimable.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_auth( st.issuer );

    // TODO: Query the snap_contract and make sure it's max supply does't exceed this token's max supply?
    statstable.modify( st, 0, [&]( auto& s ) {
        s.claimable = true;
        s.snap_contract = snap_contract;
        s.snap_id = snap_id;
        s.multiplier = multiplier;
    });
}

void airgrab::claim( account_name claimant, asset claimable ) {
    auto sym = claimable.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );
    eosio_assert( st.claimable, "Token is not claimable");

    require_auth( claimant );
    auto iter = _claims.find( claimant );
    while (iter != _claims.end()) {
        claimed_token c = *iter;
        eosio_assert( c.token.symbol.name() != claimable.symbol.name(), "Token has already been claimed by this account" );
    }

    SnapBalances snaps( st.snap_contract, st.snap_id );
    print("About to get snap for claimant");
    const auto& acctBal = snaps.get( claimant );

    eosio_assert( acctBal.amount <= st.max_supply.amount - st.supply.amount, "Amount exceeds available supply" );

    // TODO: deal with multiplier and different precision here
    asset toDrop = asset(acctBal.amount, st.supply.symbol);

    // TODO: Check that it has not expired by using the expiration block
    statstable.modify( st, 0, [&]( auto& s ) {
        s.supply.amount += acctBal.amount;
    });

    add_balance( claimant, toDrop, claimant );
    _claims.emplace( claimant, [&]( auto& c ){
        c.account = claimant;
        c.token = toDrop;
    });
}

void airgrab::clearclaim( account_name account, asset claimed ) {
    // TODO: check that the claimed asset has past it's expire_block (meaning it's no longer claimable),
    // then go delete the row in the claims table for this account so it can have it's ram back
}

void airgrab::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void airgrab::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

EOSIO_ABI( airgrab, (create)(issue)(transfer)(setclaimable)(claim)(clearclaim) )
