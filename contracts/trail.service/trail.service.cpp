#include "trail.service.hpp"

trail::trail(account_name self) : contract(self), environment(self, self) {
    if (!environment.exists()) {

        env_struct = env{
            self, //publisher
            0, //total_tokens
            0, //total_voters
            0, //total_ballots
            0 //active ballots
        };

        environment.set(env_struct, self);
    } else {
        env_struct = environment.get();
    }
}

trail::~trail() {
    if (environment.exists()) {
        environment.set(env_struct, env_struct.publisher);
    }
}

#pragma region Tokens

void trail::regtoken(asset native, account_name publisher) {
    require_auth(publisher);

    auto sym = native.symbol.name();

    stats statstable(N(eosio.token), sym);
    auto eosio_existing = statstable.find(sym);
    eosio_assert(eosio_existing == statstable.end(), "Token with symbol already exists in eosio.token" );

    registries_table registries(_self, _self);
    auto r = registries.find(sym);
    eosio_assert(r == registries.end(), "Token Registry with that symbol already exists in Trail");

    //TODO: assert only 1 token per publisher

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
    registries_table registries(_self, _self);
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

    voters_table voters(_self, _self);
    auto v = voters.find(voter);

    eosio_assert(v == voters.end(), "Voter already exists");

    voters.emplace(voter, [&]( auto& a ){
        a.voter = voter;
        a.liquid_votes = asset(0, S(4, VOTE));
        a.spent_votes = asset(0, S(4, VOTE));
    });

    env_struct.total_voters++;

    print("\nVoterID Registration: SUCCESS");
}

void trail::unregvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, _self);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter Doesn't Exist");

    auto vid = *v;  

    voters.erase(v);

    env_struct.total_voters--;

    print("\nVoterID Unregistration: SUCCESS");
}

void trail::regballot(account_name publisher, asset voting_token, uint32_t begin_time, uint32_t end_time) {
    require_auth(publisher);

    ballots_table ballots(_self, _self);

    ballots.emplace(publisher, [&]( auto& a ){
        a.ballot_id = ballots.available_primary_key();
        a.publisher = publisher;
        a.no_count = asset(0, voting_token.symbol.name());
        a.yes_count = asset(0, voting_token.symbol.name());
        a.abstain_count = asset(0, voting_token.symbol.name());
        a.unique_voters = uint32_t(0);
        a.begin_time = begin_time;
        a.end_time = end_time;
    });

    env_struct.total_ballots++;

    print("\nBallot Registration: SUCCESS");
}

void trail::unregballot(account_name publisher, uint64_t ballot_id) {
    require_auth(N(eosio.trail));

    ballots_table ballots(_self, _self);
    auto b = ballots.find(ballot_id);

    eosio_assert(b != ballots.end(), "Ballot Doesn't Exist");

    ballots.erase(b);

    env_struct.total_ballots--;

    print("\nBallot Deletion: SUCCESS");
}

void stakeforvote(account_name voter, asset amount) {
    require_auth(voter);
    eosio_assert(amount.symbol == S(4, TLOS), "only TLOS can be staked for votes");
    eosio_assert(amount >= asset(0, S(4, TLOS)), "must stake a positive amount");
    
    voters_table voters(N(eosio.trail), N(eosio.trail));
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "voter is not registered");

    auto vid = *v;
    asset new_liquid = vid.liquid_votes + asset(amount.amount, S(4, VOTE));

    voters.modify(v, 0, [&]( auto& a ) {
        a.liquid_votes = new_liquid;
    });

    action(permission_level{ voter, N(active) }, N(eosio.token), N(transfer), make_tuple(
    	voter,
        N(eosio.trail),
        amount,
        std::string("Staking TLOS for VOTE")
	)).send();

    print("\nStake For Votes: SUCCESS");
}

void unstakevotes(account_name voter, asset amount) {
    require_auth(voter);
    eosio_assert(amount.symbol == S(4, TLOS), "only TLOS can be staked for votes");
    eosio_assert(amount >= asset(0, S(4, TLOS)), "must stake a positive amount");

    voters_table voters(N(eosio.trail), N(eosio.trail));
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "voter is not registered");

    auto vid = *v;
    eosio_assert(amount <= asset(vid.liquid_votes.amount, S(4, TLOS)), "insufficient liquid votes to unstake");

    asset new_liquid = vid.liquid_votes - asset(amount.amount, S(4, VOTE));

    voters.modify(v, 0, [&]( auto& a ) {
        a.liquid_votes = new_liquid;
    });

    action(permission_level{ voter, N(active) }, N(eosio.token), N(transfer), make_tuple(
    	N(eosio.trail),
        voter,
        amount,
        std::string("Unstaking VOTE for TLOS")
	)).send();

    print("\nUnstake Votes: SUCCESS");
}

void castvote(account_name voter, uint64_t ballot_id, asset amount, uint16_t direction) {

}

#pragma endregion Voting

//EOSIO_ABI(trail, (regtoken)(unregtoken)(regvoter)(unregvoter)(addreceipt)(rmvexpvotes)(regballot)(unregballot))

extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {
        trail trailservice(self);
        if(code == self && action == N(regtoken)) {
            execute_action(&trailservice, &trail::regtoken);
        } else if (code == self && action == N(unregtoken)) {
            execute_action(&trailservice, &trail::unregtoken);
        } else if (code==self && action==N(regvoter)) {
            execute_action(&trailservice, &trail::regvoter);
        } else if (code == self && action == N(unregvoter)) {
            execute_action(&trailservice, &trail::unregvoter);
        } else if (code == self && action == N(regballot)) {
            execute_action(&trailservice, &trail::regballot);
        } else if (code == self && action == N(unregballot)) {
            execute_action(&trailservice, &trail::unregballot);
        }
    } //end apply
}; //end dispatcher
