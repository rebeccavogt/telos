/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "trail.service.hpp"

trail::trail(account_name self) : contract(self), env(self, self) {
    if (!env.exists()) {

        env_struct = environment{
            self, //publisher
            0, //total_tokens
            0 //total_voters
        };

        env.set(env_struct, self);
    } else {
        env_struct = env.get();
    }
}

trail::~trail() {
    if (env.exists()) {
        env.set(env_struct, env_struct.publisher);
    }
}


void trail::regtoken(asset native, account_name publisher) { //TODO: increment total_tokens
    //require_auth(publisher);

    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto existing = registries.find(sym);

    if (existing == registries.end()) {
        registries.emplace(publisher, [&]( auto& a ){
            a.native = native;
            a.publisher = publisher;
        });

        print("\nEmplaced Registry...");
    } else {
        print("\nRegistry Already Exists...");
    }
}


void trail::unregtoken(asset native, account_name publisher) { //TODO: decrement total_tokens
    //require_auth(publisher);
    
    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto existing = registries.find(sym);

    if (existing == registries.end()) {
        registries.erase(existing);

        print("\nEmplaced Registry...");
    } else {
        print("\nRegistry Already Exists...");
    }
}

void trail::regvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v == voters.end(), "Voter Already Exists");

    voters.emplace(voter, [&]( auto& a ){
        a.voter = voter;
        a.tlos_weight = 1; //TODO: get actual tlos weight
    });

    env_struct.total_voters++;
}

void trail::unregvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter Doesn't Exist");

    //TODO: dont erase until all votes have been rescinded or are past expiration

    voters.erase(v);

    env_struct.total_voters--;

    print("\nVoterID Successfully Erased");
}

void trail::addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, uint16_t direction, uint64_t weight, account_name voter) {
    //require_auth(voter);

    print("\nAddReceipt Called...");

    voters_table voters(_self, voter);
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "Voter Doesn't Exist");

    auto vo = *v;

    voteinfo new_vi = voteinfo{ //creating new voteinfo struct
        vote_code,
        vote_scope,
        vote_key,
        direction,
        weight
    };

    bool found = false;
    uint64_t idx = 0;

    auto itr = vo.votes_list.begin();

    for (voteinfo vi : vo.votes_list) { //Checking for existing voteinfo struct

        if (vi.vote_code == vote_code && vi.vote_scope == vote_scope && vi.vote_key == vote_key) { //Found existing
            found = true;
            print("\nExisting Receipt Found....Updating");

            //vo.votes_list[idx] = new_vi;
            auto h = vo.votes_list.erase(itr);
            auto i = vo.votes_list.insert(itr, new_vi);

            print("\nidx: ", idx);

            break;
        }

        idx++;
        itr++;
    }

    if (found == false) {

        vo.votes_list.push_back(new_vi);

        voters.modify(v, 0, [&]( auto& a ) {
            a.votes_list = vo.votes_list;
        });
    } else {

        voters.modify(v, 0, [&]( auto& a ) {
            a.votes_list = vo.votes_list;
        });
    }
}

EOSIO_ABI(trail, (regtoken)(unregtoken)(regvoter)(unregvoter)(addreceipt))