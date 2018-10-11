/**
 * Arbitration Implementation. See function bodies for further notes.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#include "arbitration.hpp"

arbitration::arbitration(account_name self) : contract(self), configs(_self, _self) {
    if (!configs.exists()) {

        vector<int64_t> fees{ 100000, 200000, 300000 };
       
       //TODO: update along with config struct
        _config = config{
            _self, //publisher
            10, //max_arbs
            5000, //default_time
            fees //fee_structure
        };

        configs.set(_config, _self);
    } else {
        _config = configs.get();
    }
}

arbitration::~arbitration() {
    if (configs.exists()) {
        configs.set(_config, _self);
    }
}

void arbitration::setconfig(uint16_t max_arbs, uint32_t default_time, vector<int64_t> fees) {
    require_auth(_self);

    //TODO: expand as struct is developed
    //NOTE: configs.set() is done in destructor
    _config = config{
        _self, //publisher
        max_arbs, //max_arbs
        default_time, //default_time
        fees
    };

    print("\nSettings Configured: SUCCESS");
}

#pragma region Arb_Elections

void arbitration::applyforarb(account_name candidate, string creds_ipfs_url) {
    require_auth(candidate);
    eosio_assert(!is_candidate(candidate), "candidate is already a candidate");
    eosio_assert(!is_arb(candidate), "candidate is already an arbitrator");

    elections_table elections(_self, _self);
    auto c = elections.find(candidate);

    elections.emplace(_self, [&]( auto& a ){
        a.candidate = candidate;
        a.credentials = creds_ipfs_url;
        a.yes_votes = uint32_t(0);
        a.no_votes = uint32_t(0);
        a.abstain_votes = uint32_t(0);
        a.expire_time = now() + _config.default_time;
        a.election_status = OPEN;
    });

    print("\nArb Application: SUCCESS");
}

void arbitration::cancelarbapp(account_name candidate) {
    require_auth(candidate);
    eosio_assert(is_candidate(candidate), "no application for given candidate");

    elections_table elections(_self, _self);
    auto c = elections.find(candidate);

    elections.erase(c); //NOTE: erase or close? remember votes are in Trail

    print("\nCancel Application: SUCCESS");
}

void arbitration::voteforarb(account_name candidate, uint16_t direction, account_name voter) {
    require_auth(voter);
    eosio_assert(direction >= 0 && direction <= 2, "direction must be between 0 and 2");
    eosio_assert(is_candidate(candidate), "no application found for given candidate");
    eosio_assert(is_election_open(candidate), "election has closed");

    //TODO: implement is_voter() in traildef.voting.hpp
    voters_table voters(N(trailservice), voter);
    auto v = voters.find(voter);
    eosio_assert(v != voters.end(), "VoterID Not Found");
    auto vid = *v;

    elections_table elections(_self, _self);
    auto e = elections.find(candidate);
    auto elec = *e;
    eosio_assert(elec.expire_time > now(), "Proposal Has Expired");

    if (vid.receipt_list.empty()) {

        print("\nReceipt List Empty...Calling TrailService to update VoterID");

        action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple(
    	    _self,      
    	    _self,
    	    candidate,
            direction,
            voter
	    )).send();

        print("\nReceipt Added. VoterID Successfully Updated");
    } else {

        print("\nSearching receipts list for existing VoteReceipt...");

        bool found = false;

        for (votereceipt r : vid.receipt_list) {
            if (r.vote_key == candidate) {

                print("\nVoteReceipt receipt found");
                found = true;

                switch (r.direction) {
                    case 0 : elec.no_votes = (elec.no_votes - uint64_t(r.weight)); break;
                    case 1 : elec.yes_votes = (elec.yes_votes - uint64_t(r.weight)); break;
                    case 2 : elec.abstain_votes = (elec.abstain_votes - uint64_t(r.weight)); break;
                }

                print("\nCalling TrailService to update VoterID...");

                action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple(
    	            _self,      
    	            _self,
    	            candidate,
                    direction,
                    voter
	            )).send();

                print("\nVoterID Successfully Updated");

                break;
            }
        }

        if (found == false) {
            print("\nVoteInfo not found in list. Calling TrailService to insert...");

            action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple(
    	        _self,      
    	        _self,
    	        candidate,
                direction,
                voter
	        )).send();

            print("\nVoterID Successfully Updated");
        }
    }

    string vote_type;
    int64_t new_weight = get_staked_tlos(voter); //NOTE: get_staked_tlos added in feature/trail-refactor commit

    switch (direction) {
        case 0 : elec.no_votes = (elec.no_votes + uint64_t(new_weight)); vote_type = "NO"; break;
        case 1 : elec.yes_votes = (elec.yes_votes + uint64_t(new_weight)); vote_type = "YES"; break;
        case 2 : elec.abstain_votes = (elec.abstain_votes + uint64_t(new_weight)); vote_type = "ABSTAIN"; break;
    }

    elections.modify(e, 0, [&]( auto& a ) {
        a.no_votes = elec.no_votes;
        a.yes_votes = elec.yes_votes;
        a.abstain_votes = elec.abstain_votes;
    });

}

void arbitration::endelection(account_name candidate) {
    eosio_assert(is_candidate(candidate), "candidate does not have an application");
    eosio_assert(is_election_expired(candidate), "election has expired");
    eosio_assert(is_election_open(candidate), "election is not open");

    elections_table elections(_self, _self);
    auto e = elections.find(candidate);
    auto elec = *e;

    uint64_t total_votes = (elec.yes_votes + elec.no_votes + elec.abstain_votes); //total votes cast on election
    uint64_t pass_thresh = ((elec.yes_votes + elec.no_votes) / 3) * 2; // 66.67% of total votes

    if (elec.yes_votes >= pass_thresh) {
        elec.election_status = PASSED;

        arbitrators_table arbitrators(_self, _self);
        auto a = arbitrators.find(candidate);

        vector<uint64_t> open_cases;
        vector<uint64_t> closed_cases;

        arbitrators.emplace(_self, [&]( auto& a ){
            a.arb = candidate
            a.arb_status = UNAVAILABLE;
            a.open_case_ids = open_cases;
            a.closed_case_ids = closed_cases;
        });

        elections.erase(e);

    } else {
        elec.election_status = FAILED;

        elections.modify(e, 0, [&]( auto& a ) {
            a.election_status = elec.election_status;
        });
    }

}
#pragma endregion Arb_Elections

#pragma region Case_Setup

void arbitration::filecase(account_name claimant, uint16_t class_suggestion, string ev_ipfs_url) {
    require_auth(claimant);
    casefiles_table casefiles(_self, _self);

    //TODO: implement deposit

    vector<account_name> arbs; //empty vector of arbitrator accounts

    casefiles.emplace(_self, [&]( auto& a ){
        a.case_id = pendingcases.available_primary_key();
        a.claimant = claimant;
        a.claims = claims;
        a.arbitrators = arbs;
        a.status = AWAITING_ARBS;
        a.last_edit = now();
    });

    //if (arb_status::) {
        //TODO: change enums to classes, check table storage?
    //}

    print("\nCased Filed: SUCCESS");
}

#pragma endregion Case_Setup

#pragma region Member_Only

void arbitration::vetoarb(uint64_t case_id, account_name arb, account_name selector) {
    require_auth(selector);
    eosio_assert(is_case(case_id), "no case for given case_id");
    eosio_assert(is_arb(arb), "account in list is not an arbitrator");

    //TODO: check that case is in AWAITING_ARBS state

}

#pragma endregion Member_Only

#pragma region Arb_Only

void arbitration::closecase(uint64_t case_id, account_name arb) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can close case");

    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");

    casefiles.erase(c);

    print("\nCase Close: SUCCESS");
}

#pragma endregion Arb_Only

#pragma region BP_Multisig_Actions
#pragma endregion BP_Multisig_Actions

#pragma region Helper_Functions

bool arbitration::is_candidate(account_name candidate) {
    elections_table elections(_self, _self);
    auto c = elections.find(candidate);

    if(c != elections.end()) {
        return true;
    }

    return false;
}

bool arbitration::is_arb(account_name arb) {
    arbitrators_table arbitrators(_self, _self);
    auto a = arbitrators.find(arb);

    if (a != arbitrators.end()) {
        return true;
    }

    return false;
}

bool arbitration::is_case(uint64_t case_id) {
    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);

    if (c != casefiles.end()) {
        return true;
    }

    return false;
}

bool arbitration::is_election_open(account_name candidate) {
    elections_table elections(_self, _self);
    auto c = elections.find(candidate);

    if(c != elections.end()) {
        auto cf = *c;

        if (cf.election_status == 0) {
            return true;
        }
    }

    return false;
}

bool arbitration::is_election_expired(account_name candidate) {
    elections_table elections(_self, _self);
    auto c = elections.find(candidate);

    if(c != elections.end()) {
        auto cf = *c;

        if (now() > cf.expire_time) {
            return true;
        }
    }

    return false;
}

#pragma endregion Helper_Functions
