/**
 * Arbitration Implementation. See function bodies for further notes.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#include "arbitration.hpp"

arbitration::arbitration(account_name self) : contract(self), configs(_self, _self) {
    if (!configs.exists()) {
       
        _config = config{
            _self, //publisher
            10, //max_arbs
            5000 //default_time
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

void arbitration::setconfig(uint16_t max_arbs, uint32_t default_time) {
    require_auth(_self);

    //TODO: expand as struct is developed
    //NOTE: configs.set() is done in destructor
    _config = config{
        _self, //publisher
        max_arbs, //max_arbs
        default_time //default_time
    };

    print("\nSettings Configured: SUCCESS");
}

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

void arbitration::closecase(uint64_t case_id, account_name arb) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can close case");

    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");

    casefiles.erase(c);

    print("\nCase Close: SUCCESS");
}

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

    elections.erase(c); //NOTE: erase or save? remember votes are in Trail

    print("\nCancel Application: SUCCESS");
}

void arbitration::voteforarb(account_name candidate, uint16_t direction, account_name voter) {
    require_auth(voter);
    eosio_assert(direction >= 0 && direction <= 2, "direction must be between 0 and 2");
    eosio_assert(is_candidate(candidate), "no application found for given candidate");

    //TODO: implement Trail voting
}

void arbitration::vetoarb(uint64_t case_id, account_name arb, account_name selector) {
    require_auth(selector);
    eosio_assert(is_case(case_id), "no case for given case_id");
    eosio_assert(is_arb(arb), "account in list is not an arbitrator");

    //TODO: check that case is in AWAITING_ARBS state

}

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


