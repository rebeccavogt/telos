/**
 * 
 * 
 * @author Craig Branscom
 */

#include "../token.registry/token.registry.hpp"

registry::registry(account_name self) : contract(self), _config(self, self) {
    if (!_config.exists()) {

        //NOTE: Developers edit here
        config = tokenconfig{
            self, //publisher
            "Telos Foundation Voting Token", //token_name
            asset(int64_t(10000), S(2, TFVT)), //max_supply
            asset(int64_t(0), S(2, TFVT)) //supply
        };

        _config.set(config, self);
    } else {
        config = _config.get();
    }
}

registry::~registry() {
    if (_config.exists()) {
        _config.set(config, config.publisher);
    }
}

void registry::mint(account_name recipient, asset tokens) {
    require_auth(config.publisher);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(config.supply + tokens <= config.max_supply, "minting would exceed allowed maximum supply");

    add_balance(recipient, tokens, config.publisher);

    config.supply = (config.supply + tokens);
}

void registry::transfer(account_name sender, account_name recipient, asset tokens) {
    require_auth(config.publisher);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(sender != recipient, "cannot transfer to self");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must transfer positive quantity");
    
    add_balance(recipient, tokens, sender);
    sub_balance(sender, tokens);
}

void registry::allot(account_name sender, account_name recipient, asset tokens) {
    require_auth(config.publisher);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(sender != recipient, "cannot allot tokens to self");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must allot positive quantity");

    sub_balance(sender, tokens);
    add_allot(sender, recipient, tokens, sender);
}

void registry::unallot(account_name sender, account_name recipient, asset tokens) {
    require_auth(config.publisher);
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must allot positive quantity");

    sub_allot(sender, recipient, tokens);
    add_balance(sender, tokens, sender);
}

void registry::claimallot(account_name sender, account_name recipient, asset tokens) {
    require_auth(recipient);
    eosio_assert(is_account(sender), "sender account does not exist");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must transfer positive quantity");
    
    sub_allot(sender, recipient, tokens);
    add_balance(recipient, tokens, recipient);
}

void registry::createwallet(account_name recipient) {
    require_auth(recipient);

    balances_table balances(config.publisher, recipient);
    auto itr = balances.find(recipient);

    eosio_assert(itr == balances.end(), "Wallet already exists for given account");

    balances.emplace(recipient, [&]( auto& a ){
        a.owner = recipient;
        a.tokens = asset(int64_t(0), config.max_supply.symbol);
    });
}

void registry::deletewallet(account_name owner) {
    require_auth(owner);

    balances_table balances(config.publisher, owner);
    auto itr = balances.find(owner);

    eosio_assert(itr != balances.end(), "Given account does not have a wallet");

    auto b = *itr;

    eosio_assert(b.tokens.amount == 0, "Cannot delete wallet unless balance is zero");

    balances.erase(itr);
}

void registry::sub_balance(account_name owner, asset tokens) {
    balances_table balances(config.publisher, owner);
    auto itr = balances.find(owner);
    auto b = *itr;

    eosio_assert(b.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    balances.modify(itr, 0, [&]( auto& a ) {
        a.tokens -= tokens;
    });
}

void registry::add_balance(account_name recipient, asset tokens, account_name payer) {
    balances_table balances(config.publisher, recipient);
    auto itr = balances.find(recipient);

    eosio_assert(itr != balances.end(), "No wallet found for recipient");

    balances.modify(itr, 0, [&]( auto& a ) {
        a.tokens += tokens;
    });
}

void registry::sub_allot(account_name owner, account_name recipient, asset tokens) {
    allotments_table allotments(config.publisher, owner);
    auto itr = allotments.find(recipient);
    auto al = *itr;

    eosio_assert(al.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(al.tokens.amount == tokens.amount ) {
        allotments.erase(itr);
    } else {
        allotments.modify(itr, 0, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }
}

void registry::add_allot(account_name sender, account_name recipient, asset tokens, account_name payer) {
    
    allotments_table allotments(config.publisher, sender);
    auto itr = allotments.find(recipient);

    if(itr == allotments.end() ) {
        allotments.emplace(payer, [&]( auto& a ){
            a.recipient = recipient;
            a.sender = sender;
            a.tokens = tokens;
        });
   } else {
        allotments.modify(itr, 0, [&]( auto& a ) {
            a.tokens += tokens;
        });
   }
}

EOSIO_ABI(registry, (mint)(transfer)(allot)(unallot)(claimallot)(createwallet)(deletewallet))
