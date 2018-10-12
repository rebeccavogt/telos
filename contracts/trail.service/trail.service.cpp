#include "trail.service.hpp"

trail::trail(account_name self) : contract(self), env_singleton(self, self) {
    if (!env_singleton.exists()) {

        env_struct = environment{
            self, //publisher
            0, //total_tokens
            0, //total_voters
            0 //total_ballots
        };

        env_singleton.set(env_struct, self);
    } else {
        env_struct = env_singleton.get();
    }
}

trail::~trail() {
    if (env_singleton.exists()) {
        env_singleton.set(env_struct, env_struct.publisher);
    }
}

#pragma region Tokens

void trail::regtoken(asset native, account_name publisher) {
    require_auth(publisher);

    auto sym = native.symbol.name();

    stats statstable(N(eosio.token), sym);
    auto eosio_existing = statstable.find(sym);
    eosio_assert(eosio_existing == statstable.end(), "Token with symbol already exists in eosio.token" );

    registries_table registries(_self, sym);
    auto r = registries.find(sym);
    eosio_assert(r == registries.end(), "Token Registry with that symbol already exists in Trail");

    registries.emplace(publisher, [&]( auto& a ){
        a.native = native;
        a.publisher = publisher;
    });

    env_struct.total_tokens++;

    print("\nToken Registration: SUCCESS");
}

void trail::unregtoken(asset native, account_name publisher) {
    require_auth(publisher);
    
    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto r = registries.find(sym);

    eosio_assert(r != registries.end(), "Token Registry does not exist.");

    registries.erase(r);

    env_struct.total_tokens--;

    print("\nToken Unregistration: SUCCESS");
}

#pragma endregion Trail_Tokens

#pragma region Voting

void trail::regvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v == voters.end(), "Voter already exists");

    voters.emplace(voter, [&]( auto& a ){
        a.voter = voter;
    });

    env_struct.total_voters++;

    print("\nVoterID Registration: SUCCESS");
}

void trail::unregvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter Doesn't Exist");

    auto vid = *v;

    eosio_assert(vid.receipt_list.size() == 0, "Can't delete until all vote receipts removed");    

    voters.erase(v);

    env_struct.total_voters--;

    print("\nVoterID Unregistration: SUCCESS");
}

void trail::addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, symbol_name vote_token, uint16_t direction, uint32_t expiration, account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "VoterID doesn't exist");
    auto vid = *v;

    int64_t new_weight = 0;

    if (vote_token == asset(int64_t(0), S(4, TLOS)).symbol.name()) { //TODO: simpler way of getting TLOS symbol_name
        int64_t new_weight = get_staked_tlos(voter);
    } else if (is_trail_token(vote_token)) {
        int64_t new_weight = get_token_balance(vote_token, voter);
    } else if (is_eosio_token(vote_token, voter)) {
        int64_t new_weight = get_eosio_token_balance(vote_token, voter);
    } else {
        print("\nNo token balance found for given symbol, defaulting to 0");
    }

    votereceipt new_vr = votereceipt{
        vote_code,
        vote_scope,
        vote_key,
        vote_token,
        direction,
        new_weight,
        expiration
    };

    auto itr = vid.receipt_list.begin();

    //search for existing receipt
    for (votereceipt r : vid.receipt_list) {
        if (r.vote_code == vote_code && r.vote_scope == vote_scope && r.vote_key == vote_key) {
            print("\nExisting receipt found. Updating...");

            auto h = vid.receipt_list.erase(itr);
            auto i = vid.receipt_list.insert(itr, new_vr);

            break;
        }
        itr++;
    }

    //if receipt not found in list
    if (itr == vid.receipt_list.end()) {
        vid.receipt_list.push_back(new_vr);
    } 

    voters.modify(v, 0, [&]( auto& a ) {
        a.receipt_list = vid.receipt_list;
    });

    print("\nVoteReceipt Addition: SUCCESS");
}

void trail::rmvexpvotes(account_name voter) {
    //TODO: set default param for limit of removals per call?
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter doesn't exist");
    auto vid = *v;

    uint32_t now_time = now(); //better name than now_time?

    auto itr = vid.receipt_list.begin();

    uint64_t votes_removed = 0;

    for (votereceipt r : vid.receipt_list) {
        if (now_time > r.expiration) {
            vid.receipt_list.erase(itr);
            votes_removed++;
        }
        itr++;
    }

    voters.modify(v, 0, [&]( auto& a ) {
        a.receipt_list = vid.receipt_list;
    });

    print("\nReceipts Removed: ", votes_removed);
}

void trail::regballot(account_name publisher) {
    require_auth(publisher);

    ballots_table ballots(_self, publisher);
    auto b = ballots.find(publisher);

    eosio_assert(b == ballots.end(), "Ballot already exists");

    ballots.emplace(publisher, [&]( auto& a ){
        a.publisher = publisher;
    });

    env_struct.total_ballots++;

    print("\nBallot Registration: SUCCESS");
}

void trail::unregballot(account_name publisher) {
    require_auth(publisher);

    ballots_table ballots(_self, publisher);
    auto b = ballots.find(publisher);

    eosio_assert(b != ballots.end(), "Ballot Doesn't Exist");

    ballots.erase(b);

    env_struct.total_ballots--;

    print("\nBallot Unregistration: SUCCESS");
}

#pragma endregion Voting

//EOSIO_ABI(trail, (regtoken)(unregtoken)(regvoter)(unregvoter)(addreceipt)(rmvexpvotes)(regballot)(unregballot))

extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {
        trail _trail(self);
        if(code == self && action == N(regtoken)) {
            execute_action(&_trail, &trail::regtoken);
        } else if (code == self && action == N(unregtoken)) {
            execute_action(&_trail, &trail::unregtoken);
        } else if (code==self && action==N(regvoter)) {
            execute_action(&_trail, &trail::regvoter);
        } else if (code == self && action == N(unregvoter)) {
            execute_action(&_trail, &trail::unregvoter);
        } else if (code == self && action == N(addreceipt)) {
            execute_action(&_trail, &trail::addreceipt);
        } else if (code == self && action == N(rmvexpvotes)) {
            execute_action(&_trail, &trail::rmvexpvotes);
        } else if (code == self && action == N(regballot)) {
            execute_action(&_trail, &trail::regballot);
        } else if (code == self && action == N(unregballot)) {
            execute_action(&_trail, &trail::unregballot);
        } else if (code == N(eosio) && action == N(delegatebw)) { //TODO: case for undelegatebw
            print("\neosio.trail received delegatebw action copy from eosio");
            //if (args.from == args.receiver) {} //only care about delegatebw to self
            //require_recipient(N(eosio.amend)); //forwards delegatebw action to eosio.amend account
            auto args = unpack_action_data<delegatebw_args>(); //NOTE: custom struct to represent delegatebw params

            voters_table voters(self, self);
            auto v = voters.find(args.from);

            print("\nvoter found: ", name{args.from});

            if (v != voters.end()) { //only forwards action if user is a registered voter
                auto vid = *v;

                for (votereceipt vr : vid.receipt_list) {
                    if (vr.expiration > now() && vr.vote_token == args.stake_cpu_quantity.symbol.name()) { //NOTE: only works when voted token is TLOS

                        //action::action(permission_level{ args.from, N(active) }, vr.vote_code, N(vote), make_tuple(
    	                    //vr.vote_key,
                            //vr.direction,
                            //args.receiver
	                    //)).send();

                        print("\nvote action sent to: ", name{vr.vote_code});
                        print(" for proposal: ", vr.vote_key);
                        print("\nsent by: ", args.from);
                    }
                }
            }

            print("\nvoter not found");
        }
    } //end apply
};
