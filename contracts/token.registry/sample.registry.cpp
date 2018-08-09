/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief Recommended TIP-5 Implementation
 * @author Craig Branscom
 * 
 * TODO: Enforce rejection of subsequent calls to init()
 * TODO: Check for matching tokens
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>
#include "token.registry.hpp"

#include <eosiolib/print.hpp>

using namespace eosio;
using namespace std;

/**
 * @brief init() is called to set the token information.
 */
void registry::init() {
    require_auth(contractowner);
    settings.set(
        setting{
            contractowner,
            asset(int64_t(10000), S(2, ITT)),
            asset(int64_t(0), S(2, ITT)),
            "Interface Test Token",
            true
        },
        contractowner
    );
}

/**
 * @brief mint() is called to mint new tokens into circulation. New tokens are sent to recipient.
 */
void registry::mint(account_name recipient, asset tokens) {
    require_auth(contractowner);
    eosio_assert(is_account(recipient), "recipient account does not exist");

    auto s = settings.get();
    eosio_assert(s.supply + tokens <= s.max_supply, "minting would exceed max_supply");

    add_balance(recipient, tokens, contractowner);

    auto new_supply = (s.supply + tokens);

    settings.set(
        setting{
            s.issuer,
            s.max_supply,
            new_supply,
            s.name,
            s.is_initialized
        },
        contractowner
    );

        print("\nnew_supply: ", new_supply);
        print("\nmax_supply: ", s.max_supply);
        print("\nissuer: ", name{s.issuer});
        print("\nname: ", s.name);
}

/**
 * @brief transfer() sends tokens from owner to recipient.
 */
void registry::transfer(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot transfer to self");
    eosio_assert(tokens.is_valid(), "invalid quantity given" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );
    
    sub_balance(owner, tokens);
    add_balance(recipient, tokens, owner);
}

/**
 * @brief allot() is called to set an allotment of tokens to be claimed later by recipient.
 */
void registry::allot(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot allot tokens to self");

    sub_balance(owner, tokens);
    
    allotments_table allotments(contractowner, owner);
    auto al = allotments.find(owner);

    if(al == allotments.end() ) {
      allotments.emplace(owner, [&]( auto& a ){
        a.owner = owner;
        a.recipient = recipient;
        a.tokens = tokens;
      });
   } else {
      allotments.modify(al, 0, [&]( auto& a ) {
        a.recipient = recipient;
        a.tokens += tokens;
      });
   }
}

/**
 * @brief transferfrom() is called to claim an allotment.
 */
void registry::transferfrom(account_name owner, account_name recipient, asset tokens) {
    require_auth(recipient);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot transfer from self to self");
    eosio_assert(tokens.is_valid(), "invalid quantity given" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );

    allotments_table allotments(contractowner, owner);
    auto itr = allotments.find(owner);
    auto al = *itr;
    eosio_assert(al.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(al.tokens.amount == tokens.amount ) {
        allotments.erase(itr);
    } else {
        allotments.modify(itr, recipient, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }

    add_balance(recipient, tokens, recipient);
}

/**
 * @brief sub_balance() is called as a shorthand way of subtracting tokens from owner's entry in the balances table.
 * 
 * NOTE: This implementation only affects the balances table. Allotments are not affected.
 */
void registry::sub_balance(account_name owner, asset tokens) {
    eosio_assert(is_account(owner), "owner account does not exist");

    balances_table balances(contractowner, owner);
    auto itr = balances.find(owner);
    auto b = *itr;
    eosio_assert(b.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(b.tokens.amount == tokens.amount ) {
        balances.erase(itr);
    } else {
        balances.modify(itr, owner, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }
}

/**
 * @brief add_balance is called as a shorthand way of adding tokens to owner's entry in the balances table.
 * 
 * NOTE: This implementation only affects the balances table. Allotments are not affected.
 */
void registry::add_balance(account_name owner, asset tokens, account_name payer) {
    eosio_assert(is_account(owner), "owner account does not exist");
    eosio_assert(is_account(payer), "payer account does not exist");

    balances_table balances(contractowner, owner);
    auto b = balances.find(owner);

    if(b == balances.end() ) {
      balances.emplace(payer, [&]( auto& a ){
        a.owner = owner;
        a.tokens = tokens;
      });
   } else {
      balances.modify(b, 0, [&]( auto& a ) {
        a.tokens += tokens;
      });
   }
}

//NOTE: sub_balance() and add_balance() are omitted from the ABI. These should only be callable by the contract itself.
EOSIO_ABI(registry, (init)(mint)(transfer)(allot)(transferfrom))
