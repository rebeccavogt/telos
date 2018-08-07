/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 * 
 *  @brief Recommended TIP-5 Implementation
 * 
 * TODO: Check for matching asset symbols
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>
#include <token.registry.hpp>

using namespace eosio;
using namespace std;

/**
 * 
 */
void registry::init() {
    settings_table token_settings(contract_owner, contract_owner);
    auto t = *token_settings.begin();

    token_settings.emplace(contract_owner, [&]( auto& a ){
        a.name = "";
        a.max_supply = 1000;
        a.supply = 10;
        a.symbol = S(2, TEST);
        a.is_initialized = true;
    });
}

/**
 * 
 */
void registry::mint(account_name recipient, asset tokens, string memo) {
    require_auth(contract_owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");

    settings_table token_settings(contract_owner, contract_owner);
    auto t = *token_settings.begin();
    eosio_assert(tokens.amount + t.supply <= t.max_supply, "minting would exceed max_supply");

    add_balance(recipient, tokens, recipient);
}

/**
 * 
 */
void registry::transfer(account_name owner, account_name recipient, asset tokens, string memo) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot transfer to self");
    eosio_assert(tokens.is_valid(), "invalid quantity given" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes" );
    
    sub_balance(owner, tokens);
    add_balance(owner, tokens, recipient);
}

/**
 * 
 */
void registry::allot(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot allot tokens to self");

    sub_balance(owner, tokens);
    
    allotments_table allotments(contract_owner, owner);
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
 * 
 */
void registry::transferfrom(account_name owner, account_name recipient, asset tokens, string memo) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot transfer from self to self");
    eosio_assert(tokens.is_valid(), "invalid quantity given" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes" );

    allotments_table allotments(contract_owner, owner);
    auto al = allotments.get(owner, "No allotment approved by owner account");
    eosio_assert(al.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(al.tokens.amount == tokens.amount ) {
        allotments.erase(al);
    } else {
        allotments.modify(al, owner, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }

    add_balance(recipient, tokens, recipient);
}

/**
 * 
 */
void registry::sub_balance(account_name owner, asset tokens) {
    eosio_assert(is_account(owner), "owner account does not exist");

    balances_table balances(contract_owner, owner);
    auto b = balances.get(owner, "No balance exists for given account");
    eosio_assert(b.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(b.tokens.amount == tokens.amount ) {
        balances.erase(b);
    } else {
        balances.modify(b, owner, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }
}

/**
 * 
 */
void registry::add_balance(account_name owner, asset tokens, account_name payer) {
    eosio_assert(is_account(owner), "owner account does not exist");
    eosio_assert(is_account(payer), "payer account does not exist");

    balances_table balances(contract_owner, owner);
    auto b = balances.find(owner);

    if(b == balances.end() ) {
      balances.emplace(payer, [&]( auto& a ){
        a.tokens = tokens;
      });
   } else {
      balances.modify(b, 0, [&]( auto& a ) {
        a.tokens += tokens;
      });
   }
}

EOSIO_ABI(registry, (mint)(transfer)(allot)(transferfrom))