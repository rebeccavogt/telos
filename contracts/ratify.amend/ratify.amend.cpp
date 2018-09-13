#include "ratify.amend.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

ratifyamend::ratifyamend(account_name self) : contract(self), thresh(self, self) {
    if (!thresh.exists()) {

        thresh_struct = threshold{
            _self, //publisher
            0, //initial quorum_threshold
            120 //expiration_length in seconds
        };

        thresh.set(thresh_struct, _self);
    } else {

        thresh_struct = thresh.get();     
        update_thresh();
        thresh.set(thresh_struct, _self);
    }
}

ratifyamend::~ratifyamend() {
    if (thresh.exists()) {
        thresh.set(thresh_struct, _self);
    }
}

void ratifyamend::insertdoc(string title, vector<string> clauses) {
    require_auth(_self); //Only contract owner can insert new document
    
    documents_table documents(_self, _self);

    uint64_t doc_id = documents.available_primary_key();

    documents.emplace(_self, [&]( auto& a ){
        a.id = doc_id;
        a.title = title;
        a.clauses = clauses;
    });

    print("\nDocument Emplaced");
    print("\nDocument ID: ", doc_id);
}

void ratifyamend::propose(string title, string ipfs_url, uint64_t document_id, uint64_t clause_id, account_name proposer) {
    require_auth(proposer);

    //eosio_assert(document_id >= 0 && clause_id >= 0, "Document and Clause ID cannot be less than 0"); //irrellevant?

    documents_table documents(_self, _self);
    auto d = documents.find(document_id);

    eosio_assert(d != documents.end(), "Document Not Found");
    print("\nDocument Found");

    auto doc_struct = *d;

    eosio_assert(doc_struct.clauses.size() >= clause_id, "Clause Not Found"); //TODO: consider revising to vector.at()

    action(permission_level{ proposer, N(active) }, N(eosio.token), N(transfer), make_tuple( //NOTE: susceptible to ram-drain bug
    	proposer,
        N(trailservice),
        asset(int64_t(1000000), S(4, TLOS)),
        std::string("Ratify/Amend Proposal Fee")
	)).send();

    proposals_table proposals(_self, _self);

    uint64_t prop_id = proposals.available_primary_key();
    uint32_t prop_time = now();

    proposals.emplace(proposer, [&]( auto& a ){
        a.id = prop_id;
        a.document_id = document_id;
        a.clause_id = clause_id;
        a.title = title;
        a.ipfs_url = ipfs_url;
        a.yes_count = 0;
        a.no_count = 0;
        a.abstain_count = 0;
        a.proposer = proposer;
        a.expiration = prop_time + thresh_struct.expiration_length;
        a.status = "OPEN";
    });

    print("\nProposal Emplaced");
    print("\nProposal ID: ", prop_id);
}

void ratifyamend::vote(uint64_t proposal_id, uint16_t vote, account_name voter) {
    require_auth(voter);
    eosio_assert(vote >= 0 && vote <= 2, "Invalid Vote. [0 = ABSTAIN, 1 = YES, 2 = NO]");

    voters_table voters(N(trailservice), voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "VoterID Not Found");
    
    print("\nVoterID Found");
    auto vo = *v;

    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    
    print("\nProposal Found");
    auto po = *p;

    eosio_assert(po.expiration > now(), "Proposal Has Expired");

    if (vo.votes_list.empty()) {

        print("\nVoteInfo Stack Empty...Calling TrailService to update VoterID");

        action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple( //TODO: likely wrong authority...
    	    _self,      
    	    _self,
    	    po.id,
            vote,
            uint64_t(1),
            voter
	    )).send();

        print("\nVoterID Successfully Updated");
    } else {

        print("\nSearching receipts list for existing VoteInfo...");

        bool found = false;

        for (voteinfo receipt : vo.votes_list) {
            if (receipt.vote_key == proposal_id) {

                print("\nVoteInfo receipt found");
                found = true;

                switch (receipt.direction) { //TODO: remove by old weight, not just decrement
                    case 0 : po.abstain_count--; break;
                    case 1 : po.yes_count--; break;
                    case 2 : po.no_count--; break;
                }

                print("\nCalling TrailService to update VoterID...");

                action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple(
    	            _self,      
    	            _self,
    	            po.id,
                    vote,
                    uint64_t(1),
                    voter
	            )).send();

                print("\nVoterID Successfully Updated");

                break; //necessary? may help reduce computation
            }
        }

        if (found == false) {
            print("\nVoteInfo not found in list. Calling TrailService to insert...");

            action(permission_level{ voter, N(active) }, N(trailservice), N(addreceipt), make_tuple(
    	        _self,      
    	        _self,
    	        po.id,
                vote,
                uint64_t(1),
                voter
	        )).send();

            print("\nVoterID Successfully Updated");
        }
    }

    //TODO: increase by new vote weight, not just increment
    switch (vote) {
        case 0 : po.abstain_count++; break;
        case 1 : po.yes_count++; break;
        case 2 : po.no_count++; break;
    }

    proposals.modify(p, 0, [&]( auto& a ) {
        a.abstain_count = po.abstain_count;
        a.yes_count = po.yes_count;
        a.no_count = po.no_count;
    });

    string vote_type;
    switch (vote) {
        case 0 : vote_type = "ABSTAIN"; break;
        case 1 : vote_type = "YES"; break;
        case 2 : vote_type = "NO"; break;
    }

    print("\n\nVOTE SUCCESSFUL");
    print("\nYour Vote: ", vote_type);
    print("\nVote Weight: ", 1); //TODO: update with real weight of vote cast
}

void ratifyamend::close(uint64_t proposal_id) { // TODO: add calling account?
    
    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    
    print("\nProposal Found");
    auto po = *p;

    eosio_assert(po.expiration <= now(), "Voting Window Still Open");
    eosio_assert(po.status == "OPEN", "Proposal Already Closed");

    auto total_votes = (po.yes_count + po.no_count + po.abstain_count);
    auto pass_thresh = (((po.yes_count + po.no_count) / 3) * 2) + 1; // 2/3 +1

    print("\npass_thresh: ", pass_thresh);

    if (total_votes >= thresh_struct.quorum_threshold) {
        
        if (po.yes_count >= pass_thresh) {
            
            proposals.modify(p, 0, [&]( auto& a ) {
                a.status = "PASSED";
            });

            action(permission_level{ po.proposer, N(active) }, N(eosio.token), N(transfer), make_tuple( //NOTE: susceptible to ram-drain bug
    	        N(trailservice),
                po.proposer,
                asset(int64_t(1000000), S(4, TLOS)),
                std::string("Ratify/Amend Proposal Fee Refund")
	        )).send();

            print("\nProposal Passed");
            print("\nRefund Sent to Proposer");
        } else {

            proposals.modify(p, 0, [&]( auto& a ) {
                a.status = "FAILED";
            });

            print("\nProposal Failed Due To Insufficient YES Votes");
        }
    } else {
        
        proposals.modify(p, 0, [&]( auto& a ) {
            a.status = "FAILED";
        });

        print("\nProposal Failed Due To Insufficient Quorum");
    }
    
}

void ratifyamend::update_thresh() { //NOTE: tentative values

    environment_singleton env(N(trailservice), N(trailservice)); //TODO: change code to trailservice account
    environment e = env.get();

    uint64_t new_quorum = e.total_voters / 4; //25% of all registered voters
    //uint64_t new_pass = (new_quorum / 2) + 1; // 50% + 1 of quorum

    thresh_struct.quorum_threshold = new_quorum;
    //thresh_struct.pass_threshold = new_pass;

    print("\nEnvironment Thresholds Updated");
}

EOSIO_ABI( ratifyamend, (insertdoc)(propose)(vote)(close))