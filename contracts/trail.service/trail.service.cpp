#include "trail.service.hpp"

trail::trail(account_name self) : contract(self), environment(self, self) {
    if (!environment.exists()) {

        env_struct = env{
            self, //publisher
            0, //total_tokens
            0, //total_voters
            0, //total_ballots
            asset(0, S(4, VOTE)), //vote_supply
            now() //time_now
        };

        environment.set(env_struct, self);
    } else {
        env_struct = environment.get();
        env_struct.time_now = now();
    }
}

trail::~trail() {
    if (environment.exists()) {
        environment.set(env_struct, env_struct.publisher);
    }
}

#pragma region Token_Actions

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

#pragma endregion Token_Actions

#pragma region Voting_Actions

void trail::regvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, _self);
    auto v = voters.find(voter);

    eosio_assert(v == voters.end(), "Voter already exists");

    voters.emplace(voter, [&]( auto& a ){
        a.voter = voter;
        a.votes = asset(0, S(4, VOTE));
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

    env_struct.vote_supply -= vid.votes;

    voters.erase(v);

    env_struct.total_voters--;

    print("\nVoterID Unregistration: SUCCESS");
}

void trail::regballot(account_name publisher, asset voting_token, uint32_t begin_time, uint32_t end_time, string info_url) {
    require_auth(publisher);

    //TODO: check for voting_token existence?

    ballots_table ballots(_self, _self);

    ballots.emplace(publisher, [&]( auto& a ){
        a.ballot_id = ballots.available_primary_key();
        a.publisher = publisher;
        a.info_url = info_url;
        a.no_count = asset(0, voting_token.symbol);
        a.yes_count = asset(0, voting_token.symbol);
        a.abstain_count = asset(0, voting_token.symbol);
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

    //TODO: prevent deletion of open ballot?

    ballots.erase(b);

    env_struct.total_ballots--;

    print("\nBallot Deletion: SUCCESS");
}

void trail::getvotes(account_name voter, uint32_t lock_period) {
    require_auth(voter);
    eosio_assert(lock_period >= MIN_LOCK_PERIOD, "lock period must be greater than 1 day (86400 secs)");
    eosio_assert(lock_period <= MAX_LOCK_PERIOD, "lock period must be less than 3 months (7,776,000 secs)");

    //TODO: implement vote_levy

    asset max_votes = get_liquid_tlos(voter) + get_staked_tlos(voter);
    eosio_assert(max_votes.symbol == S(4, TLOS), "only TLOS can be used to get VOTEs"); //NOTE: redundant?
    eosio_assert(max_votes > asset(0, S(4, TLOS)), "must get a positive amount of VOTEs"); //NOTE: redundant?
    
    voters_table voters(N(eosio.trail), N(eosio.trail));
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "voter is not registered");

    auto vid = *v;
    eosio_assert(now() >= vid.release_time, "cannot get more votes until lock period is over");

    voters.modify(v, 0, [&]( auto& a ) {
        a.votes = asset(amount.amount, S(4, VOTE)); //mirroring TLOS amount, not spending/locking it up
        a.release_time = now() + lock_period;
    });

    print("\nRegister For Votes: SUCCESS");
}

void trail::castvote(account_name voter, uint64_t ballot_id, uint16_t direction) {
    require_auth(voter);
    eosio_assert(direction >= uint16_t(0) && direction <= uint16_t(2), "Invalid Vote. [0 = NO, 1 = YES, 2 = ABSTAIN]");

    voters_table voters(N(eosio.trail), N(eosio.trail));
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "voter is not registered");

    auto vid = *v;

    ballots_table ballots(N(eosio.trail), N(eosio.trail));
    auto b = ballots.find(ballot_id);
    eosio_assert(b != ballots.end(), "ballot with given ballot_id doesn't exist");

    auto bal = *b;
    eosio_assert(env_struct.time_now >= bal.begin_time && env_struct.time_now <= bal.end_time, "ballot voting window not open");
    //eosio_assert(vid.release_time >= bal.end_time, "can only vote for ballots that end before your lock period is over...prevents double voting!");

    votereceipts_table votereceipts(N(eosio.trail), voter);
    auto vr_itr = votereceipts.find(ballot_id);
    //eosio_assert(vr_itr == votereceipts.end(), "voter has already cast vote for this ballot");

    if (vr_itr == votereceipts.end()) { //NOTE: voter hasn't voted on ballot before
        votereceipts.emplace(voter, [&]( auto& a ){
            a.ballot_id = ballot_id;
            a.direction = direction;
            a.weight = vid.votes;
            a.expiration = bal.end_time;
        });
    } else { //NOTE: vote for ballot_id already exists
        auto vr = *vr_itr;

        if (vr.expiration == bal.end_time && vr.direction != direction) { //NOTE: vote different and for same cycle

            switch (vr.direction) {
                case 0 : bal.no_count -= vid.votes; break;
                case 1 : bal.yes_count -= vid.votes; break;
                case 2 : bal.abstain_count -= vid.votes; break;
            }

            votereceipts.modify(vr_itr, 0, [&]( auto& a ) {
                a.direction = direction;
                a.weight = vid.votes;
            });

            print("\nVote Recast: SUCCESS");
        } else if (vr.expiration < bal.end_time) { //NOTE: vote for new cycle
            votereceipts.modify(vr_itr, 0, [&]( auto& a ) {
                a.direction = direction;
                a.weight = vid.votes;
                a.expiration = bal.end_time;
            });
        }
    }

    switch (direction) {
        case 0 : bal.no_count += vid.votes; break;
        case 1 : bal.yes_count += vid.votes; break;
        case 2 : bal.abstain_count += vid.votes; break;
    }

    ballots.modify(b, 0, [&]( auto& a ) {
        a.no_count = bal.no_count;
        a.yes_count = bal.yes_count;
        a.abstain_count = bal.abstain_count;
        a.unique_voters += uint32_t(1);
    });

    print("\nVote: SUCCESS");
}

void trail::closevote(account_name publisher, uint64_t ballot_id, bool pass) {
    require_auth(publisher);

    ballots_table ballots(N(eosio.trail), N(eosio.trail));
    auto b = ballots.find(ballot_id);
    eosio_assert(b != ballots.end(), "ballot with given ballot_id doesn't exist");

    auto bal = *b;
    eosio_assert(now() >= bal.end_time, "ballot voting window still open");

    ballots.modify(b, 0, [&]( auto& a ) {
        a.status = pass;
    });

}

void trail::nextcycle(account_name publisher, uint64_t ballot_id, uint32_t new_begin_time, uint32_t new_end_time) {
    require_auth(publisher);

    ballots_table ballots(_self, _self);
    auto b = ballots.find(ballot_id);
    eosio_assert(b != ballots.end(), "Ballot Doesn't Exist");

    ballots.emplace(publisher, [&]( auto& a ){
        a.no_count = asset(0, voting_token.symbol);
        a.yes_count = asset(0, voting_token.symbol);
        a.abstain_count = asset(0, voting_token.symbol);
        a.unique_voters = uint32_t(0);
        a.begin_time = new_begin_time;
        a.end_time = new_end_time;
        a.status = false;
    });

}

void trail::deloldvotes(account_name voter, uint16_t num_to_delete) {
    require_auth(voter);

    votereceipts_table votereceipts(N(eosio.trail), voter);
    auto itr = votereceipts.begin();

    while (itr != votereceipts.end() && num_to_delete > 0) {
        if (itr->expiration < env_struct.time_now) { //NOTE: votereceipt has expired
            itr = votereceipts.erase(itr); //NOTE: returns iterator to next element
            num_to_delete--;
        } else {
            itr++;
        }
    }

}

#pragma endregion Voting_Actions

#pragma region Reactions



#pragma endregion Reactions

//EOSIO_ABI(trail, )

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
        } else if (code == self && action == N(getvotes)) {
            execute_action(&trailservice, &trail::getvotes);
        } else if (code == self && action == N(castvote)) {
            execute_action(&trailservice, &trail::castvote);
        } else if (code == self && action == N(closevote)) {
            execute_action(&trailservice, &trail::closevote);

            // auto args = unpack_action_data<closevote_args>();
            // if (is_ballot_publisher(code, args.ballot_id)) {
            //     execute_action(&trailservice, &trail::closevote);
            // } else {
            //     eosio_exit(0); //ends processing, exits without calling destructor
            // }

        } else if (code == N(eosio.token) && action == N(transfer)) { //NOTE: updates vote_levy after transfers
            //TODO: add require_recipient to token contract
        }
    } //end apply
}; //end dispatcher
