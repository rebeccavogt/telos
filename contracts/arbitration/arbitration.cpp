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

        for (auto& r : vid.receipt_list) {
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
            a.arb = candidate;
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
	eosio_assert(class_suggestion >= UNDECIDED && class_suggestion <= MISC, "class suggestion must be between 0 and 14"); //TODO: improve this message to include directions
	validate_ipfs_url(ev_ipfs_url);

	action(permission_level{ claimant, N(active)}, N(eosio.token), N(transfer), make_tuple(
		claimant,
		_self,
		asset(int64_t(1000000), S(4, TLOS)), //TODO: Check initial filing fee
		std::string("Arbitration Case Filing Fee")
	)).send();

	casefiles_table casefiles(_self, _self);
    vector<account_name> arbs; //empty vector of arbitrator accounts
	vector<claim> claims;
	auto case_id = casefiles.available_primary_key();
    casefiles.emplace(_self, [&]( auto& a ){
        a.case_id = case_id;
        a.claimant = claimant;
		a.respondant = 0;
        a.claims = claims;
        a.arbitrators = arbs;
        a.case_status = CASE_SETUP;
        a.last_edit = now();
    });

	addclaim(case_id, class_suggestion, ev_ipfs_url, claimant);

    print("\nCased Filed!");
}

void arbitration::addclaim(uint64_t case_id, uint16_t class_suggestion, string ev_ipfs_url, account_name claimant) {
	require_auth(claimant);
	eosio_assert(class_suggestion >= UNDECIDED && class_suggestion <= MISC, "class suggestion must be between 0 and 14"); //TODO: improve this message to include directions
	validate_ipfs_url(ev_ipfs_url);

	casefiles_table casefiles(_self, _self);
	auto c = casefiles.get(case_id, "Case Not Found");
	print("\nProposal Found!");

	require_auth(c.claimant);
	eosio_assert(c.case_status == CASE_SETUP, "claims cannot be added after CASE_SETUP is complete.");

	vector<uint64_t> accepted_ev_ids;

	auto new_claims = c.claims;
	new_claims.emplace_back(claim { class_suggestion, vector<string>{ev_ipfs_url}, accepted_ev_ids, UNDECIDED });
	casefiles.modify(c, 0, [&](auto& a) { 
		a.claims = new_claims;
	});

	print("\nClaim Added!");
}

void arbitration::removeclaim(uint64_t case_id, uint16_t claim_num, account_name claimant) {
	require_auth(claimant);

	casefiles_table casefiles(_self, _self);
	auto c = casefiles.get(case_id, "Case Not Found");
	print("\nProposal Found!");

	require_auth(c.claimant);
	eosio_assert(c.case_status == CASE_SETUP, "claims cannot be removed after CASE_SETUP is complete.");

	vector<claim> new_claims = c.claims;
	eosio_assert(new_claims.size() > 0, "no claims to remove");
	eosio_assert(claim_num < new_claims.size() - 1, "claim number does not exist");
	new_claims.erase(new_claims.begin() + claim_num);

	casefiles.modify(c, 0, [&](auto& a) {
		a.claims = new_claims;
	});

	print("\nClaim Removed!");
}

void arbitration::shredcase(uint64_t case_id, account_name claimant) {
	require_auth(claimant);

	casefiles_table casefiles(_self, _self);
	auto c_itr = casefiles.find(case_id);
	print("\nProposal Found!");
	eosio_assert(c_itr != casefiles.end(), "Case Not Found");

	require_auth(c_itr->claimant);
	eosio_assert(c_itr->case_status == CASE_SETUP, "cases can only be shredded during CASE_SETUP");

	casefiles.erase(c_itr);

	print("\nCase Shredded!");
}

void arbitration::readycase(uint64_t case_id, account_name claimant) {
	require_auth(claimant);

	casefiles_table casefiles(_self, _self);
	auto c = casefiles.get(case_id, "Case Not Found");

	require_auth(c.claimant);
	eosio_assert(c.case_status == CASE_SETUP, "cases can only be readied during CASE_SETUP");
	eosio_assert(c.claims.size() >= 1, "cases must have atleast one claim");

	casefiles.modify(c, 0, [&](auto& a) {
		c.case_status = AWAITING_ARBS;
	});

	print("\nCase Readied!");
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

void arbitration::closecase(uint64_t case_id, account_name arb, string ipfs_url) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can close case");
    validate_ipfs_url(ipfs_url);

    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");
    eosio_assert(c->case_status == ENFORCEMENT, "case hasn't been enforced");

    auto arb_case = std::find(c->arbitrators.begin(), c->arbitrators.end(), arb);
    eosio_assert(arb_case != c->arbitrators.end(), "arbitrator isn't selected for this case.");

    auto new_ipfs_list = c->findings_ipfs;
	new_ipfs_list.emplace_back(ipfs_url);

    casefiles.modify(c, 0, [&](auto& cf) {
       cf.findings_ipfs = new_ipfs_list;
       cf.case_status = COMPLETE;
       cf.last_edit = now();
    });    

    print("\nCase Close: SUCCESS");
}

void arbitration::dismisscase(uint64_t case_id, account_name arb, string ipfs_url) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can dismiss case");
    eosio_assert(is_case(case_id), "no case for given case_id");
    validate_ipfs_url(ipfs_url);

    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");

    auto arb_case = std::find(c->arbitrators.begin(), c->arbitrators.end(), arb);
    eosio_assert(arb_case != c->arbitrators.end(), "arbitrator isn't selected for this case.");
    eosio_assert(c->case_status == CASE_INVESTIGATION, "case is dismissed or complete");
    
    auto new_ipfs_list = c->findings_ipfs;
	new_ipfs_list.emplace_back(ipfs_url);

    casefiles.modify(c, 0, [&](auto& cf) {
        cf.findings_ipfs = new_ipfs_list;
        cf.case_status = DISMISSED;
        cf.last_edit = now();
    });

    print("\nCase Dismissed: SUCCESS");
}

void arbitration::dismissev(uint64_t case_id, uint16_t claim_num, uint16_t ev_num, account_name arb, string ipfs_url) {
    //NOTE: moves to dismissed_evidence table   


}

void arbitration::acceptev(uint64_t case_id, uint16_t claim_num, uint16_t ev_num, account_name arb) {
    //NOTE: moves to evidence_table and assigns ID
    // require_auth(arb);
    // eosio_assert(is_arb(arb), "only arbitrator can dismiss case");
    // eosio_assert(is_case(case_id), "no case for given case_id");



}

void arbitration::arbstatus(uint16_t new_status, account_name arb) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can dismiss case");
    
    arbitrators_table arbitrators(_self, _self);
    auto ptr_arb = arbitrators.find(arb);
    
    eosio_assert(new_status >= 0 || new_status <= 3, "arbitrator status doesn't exist");

    if(ptr_arb != arbitrators.end()) {
        arbitrators.modify(ptr_arb, 0, [&](auto& a) {
            a.arb_status = new_status;
        });
    }

    print("\nArbitrator status updated: SUCCESS");
}

void arbitration::casestatus(uint64_t case_id, uint16_t new_status, account_name arb) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can dismiss case");
    eosio_assert(is_case(case_id), "no case for given case_id");
    
    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");
    
    eosio_assert(c->case_status != DISMISSED || c->case_status != COMPLETE, "case is dismissed or complete");

    auto arb_case = std::find(c->arbitrators.begin(), c->arbitrators.end(), arb);
    eosio_assert(arb_case != c->arbitrators.end(), "arbitrator isn't selected for this case.");

    casefiles.modify(c, 0, [&](auto& cf) {
        cf.case_status = new_status;
        cf.last_edit = now();
    });

    print("\nCase updated: SUCCESS");
}

void arbitration::changeclass(uint64_t case_id,  uint16_t claim_index, uint16_t new_class, account_name arb) {
    require_auth(arb);
    eosio_assert(is_arb(arb), "only arbitrator can dismiss case");
    eosio_assert(is_case(case_id), "no case for given case_id");
    
    casefiles_table casefiles(_self, _self);
    auto c = casefiles.find(case_id);
    eosio_assert(c != casefiles.end(), "no case found for given case_id");

    eosio_assert(claim_index < 0 || claim_index > c->claims.size() - 1, "claim_index is out of range");

    vector<claim> new_claims = c->claims;
    new_claims[claim_index].class_suggestion = new_class;
    
    casefiles.modify(c, 0, [&](auto& cs) {
       cs.claims = new_claims;
       cs.last_edit = now();
    });

    print("\nClaim updated: SUCCESS");
}

void arbitration::recuse(uint64_t case_id, string rationale, account_name arb) {
    
}

#pragma endregion Arb_Only

#pragma region BP_Multisig_Actions

void arbitration::dismissarb(account_name arb) {
	require_auth2(N(eosio.prods), N(active)); //REQUIRES 2/3+1 Vote from eosio.prods for MSIG

	arbitrators_table arbitrators(_self, _self);
	auto a = arbitrators.get(arb, "Arbitrator Not Found");

	eosio_assert(a.arb_status != INACTIVE, "Arbitrator is already inactive");

	arbitrators.modify(a, 0, [&](auto& a) {
		a.arb_status = INACTIVE;
	});

	print("\nArbitrator Dismissed!");
}

#pragma endregion BP_Multisig_Actions

#pragma region Helper_Functions

void arbitration::validate_ipfs_url(string ipfs_url) {
	//TODO: Base58 character checker 
	eosio_assert(!ipfs_url.empty(), "ev_ipfs_url cannot be empty, evidence for claims must be submitted.");
	eosio_assert(ipfs_url.length() == 53 && ipfs_url.substr(0, 5) == "/ipfs/", "invalid ipfs string, valid schema: /ipfs/<hash>/");
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
