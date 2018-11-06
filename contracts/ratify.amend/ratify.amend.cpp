#include "ratify.amend.hpp"

#include <eosiolib/print.hpp>

ratifyamend::ratifyamend(account_name self) : contract(self), configs(self, self) {
    if (!configs.exists()) {

        configs_struct = config{
            _self, //publisher
            uint32_t(5000) //expiration_length in seconds (default is 5,000,000 or ~58 days)
        };

        configs.set(configs_struct, _self);
    } else {
        configs_struct = configs.get();     
    }
}

// ratifyamend::~ratifyamend() {
    
// }

void ratifyamend::insertdoc(string title, vector<string> clauses) {
    require_auth(_self); //only contract owner can insert new document
    
    documents_table documents(_self, _self);

    uint64_t doc_id = documents.available_primary_key();

    documents.emplace(_self, [&]( auto& a ){
        a.id = doc_id;
        a.title = title;
        a.clauses = clauses;
    });

    print("\nDocument Insertion: SUCCESS");
    print("\nAssigned Document ID: ", doc_id);
}

void ratifyamend::makeproposal(string prop_title, uint64_t doc_id, uint8_t new_clause_num, string new_ipfs_url, account_name proposer) {
    require_auth(proposer);

    documents_table documents(_self, _self);
    auto d = documents.find(doc_id);
    eosio_assert(d != documents.end(), "Document Not Found");
    auto doc_struct = *d;

    //eosio_assert(new_clause_ids.size() == new_ipfs_urls.size(), "Clause ID vector and IPFS url vector sizes must match");

    // auto doc_size = doc_struct.clauses.size();
    // int16_t last_clause_id = -1;
    // auto last_ipfs_url = 0;

    // for (int i = 0; i < new_clause_ids.size(); i++) {
    //     eosio_assert(new_clause_ids.at(i) > last_clause_id, "Clause IDs Must Be In Ascending Order");
    //     last_clause_id = new_clause_ids.at(i);
    //     eosio_assert(new_clause_ids.at(i) <= (doc_size + 1), "Invalid Clause Number");
        
    //     last_ipfs_url++;
    //     if (new_clause_ids.at(i) == (doc_size + 1)) { //if clause is new, increase doc_size
    //         doc_size++;
    //     }
    // }

    //NOTE: 100.0000 TLOS fee, refunded if proposal passes or meets specified lower thresholds
    action(permission_level{ proposer, N(active) }, N(eosio.token), N(transfer), make_tuple(
    	proposer,
        _self,
        asset(int64_t(1000000), S(4, TLOS)),
        std::string("Ratify/Amend Proposal Fee")
	)).send();

    proposals_table proposals(_self, _self);
    auto prop_id = proposals.available_primary_key();

    vector<uint8_t> clause_nums; 
    clause_nums.push_back(new_clause_num);
    
    vector<string> clause_urls;
    clause_urls.push_back(new_ipfs_url);

    proposals.emplace(proposer, [&]( auto& a ){
        a.proposal_id = prop_id;
        a.ballot_id = 0;
        a.proposer = proposer;

        a.document_id = doc_id;
        a.proposal_title = prop_title;
        a.new_clause_nums = clause_nums;
        a.new_ipfs_urls = clause_urls;

        a.begin_time = 0;
        a.end_time = 0;
        a.status = 0;
    });

    print("\nProposal: SUCCESS");
    print("\nAssigned Proposal ID: ", prop_id);
}

/*
void ratifyamend::vote(uint64_t vote_code, uint64_t vote_scope, uint64_t proposal_id, uint16_t direction, uint32_t expiration, account_name voter) {
    require_auth(voter);
    eosio_assert(direction >= 0 && direction <= 2, "Invalid Vote. [0 = NO, 1 = YES, 2 = ABSTAIN]");
    eosio_assert(is_voter(voter), "voter is not registered");

    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    auto prop = *p;
    eosio_assert(prop.expiration >= now(), "Proposal Has Expired");

    eosio_assert(expiration == prop.expiration, "expiration does not match proposal");
    eosio_assert(vote_code == _self, "vote_code must be eosio.amend");
    eosio_assert(vote_scope == _self, "vote_scope must be eosio.amend");

    require_recipient(N(eosio.trail));
    print("\nVote sent to Trail");
}
*/

/*
void ratifyamend::processvotes(uint64_t vote_code, uint64_t vote_scope, uint64_t proposal_id, uint16_t loop_count) {
    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    auto prop = *p;
    eosio_assert(prop.expiration < now(), "Proposal is still open");
    
    receipts_table votereceipts(N(eosio.trail), N(eosio.trail));
    auto by_code = votereceipts.get_index<N(bycode)>();
    auto itr = by_code.lower_bound(vote_code);

    eosio_assert(itr != by_code.end(), "no votes to process");
    print("\neosio.amend processing votes...");

    uint64_t loops = 0;
    uint64_t unique_voters = 0;
    int64_t new_no_votes = 0;
    int64_t new_yes_votes = 0;
    int64_t new_abs_votes = 0;

    while(itr->vote_code == vote_code && loops < loop_count) { //loops variable to limit cpu/net expense per call
        
        if (itr->vote_scope == vote_scope &&
            itr->prop_id == proposal_id &&
            now() > itr->expiration) {

            print("\nvr found...counting...");
            
            switch (itr->direction) {
                case 0 : new_no_votes += itr->weight.amount; break;
                case 1 : new_yes_votes += itr->weight.amount; break;
                case 2 : new_abs_votes += itr->weight.amount; break;
            }

            unique_voters++;
        }
        loops++;
        itr++;
    }

    proposals.modify(p, 0, [&]( auto& a ) {
        a.no_count += asset(new_no_votes);
        a.yes_count += asset(new_yes_votes);
        a.abstain_count += asset(new_abs_votes);
        a.total_voters += unique_voters;
    });

    print("\nloops processed: ", loops);
    print("\nnew no votes: ", asset(new_no_votes));
    print("\nnew yes votes: ", asset(new_yes_votes));
    print("\nnew abstain votes: ", asset(new_abs_votes));

    require_recipient(N(eosio.trail));
}
*/

void ratifyamend::addclause(uint64_t prop_id, uint8_t new_clause_num, string new_ipfs_url, account_name proposer) {
    require_auth(proposer);

    proposals_table proposals(_self, _self);
    auto p = proposals.find(prop_id);
    eosio_assert(p != proposals.end(), "proposal doesn't exist");
    auto prop = *p;

    eosio_assert(prop.proposer == proposer, "can't add clauses to proposal you don't own");
    eosio_assert(prop.status == 0, "proposal is past stage allowing clause addtions");

    prop.new_clause_nums.push_back(new_clause_num);
    prop.new_ipfs_urls.push_back(new_ipfs_url);

    proposals.modify(p, 0, [&]( auto& a ) {
        a.new_clause_nums = prop.new_clause_nums;
        a.new_ipfs_urls = prop.new_ipfs_urls;
    });

    print("\nAdd Clause: SUCCESS");
}

void ratifyamend::linkballot(uint64_t prop_id, uint64_t ballot_id, account_name proposer) {
    require_auth(proposer);

    proposals_table proposals(_self, _self);
    auto p = proposals.find(prop_id);
    eosio_assert(p != proposals.end(), "proposal with given id doesn't exist");
    auto prop = *p;

    ballots_table ballots(N(eosio.trail), N(eosio.trail));
    auto b = ballots.find(ballot_id);
    eosio_assert(b != ballots.end(), "ballot with given id doesn't exist");
    auto bal = *b;

    eosio_assert(bal.publisher == proposer, "cannot link to a ballot made by another account");

    proposals.modify(p, 0, [&]( auto& a ) {
        a.ballot_id = bal.ballot_id;

        a.begin_time = bal.begin_time;
        a.end_time = bal.end_time;
    });

    print("\nBallot Link: SUCCESS");
}

void ratifyamend::closeprop(uint64_t proposal_id, account_name proposer) {
    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    auto prop = *p;

    ballots_table ballots(N(eosio.trail), N(eosio.trail));
    auto b = ballots.find(prop.ballot_id);
    eosio_assert(b != ballots.end(), "Ballot ID doesn't exist");
    auto bal = *b;

    eosio_assert(bal.end_time < now(), "Proposal is still open");
    eosio_assert(prop.status == 0 && bal.status == 0, "Proposal is already closed");

    environment_singleton environment(N(eosio.trail), N(eosio.trail));
    auto e = environment.get();

    asset total_votes = (bal.yes_count + bal.no_count + bal.abstain_count); //total votes cast on proposal

    //pass thresholds
    uint64_t quorum_thresh = (e.total_voters / 20); // 5% of all registered voters
    asset pass_thresh = ((bal.yes_count + bal.no_count) / 3) * 2; // 66.67% yes votes over no votes

    //refund thresholds - both must be met for a refund - proposal pass triggers automatic refund
    uint64_t q_refund_thresh = (e.total_voters / 25); //4% of all registered voters
    asset p_refund_thresh = total_votes / 4; //25% yes votes of total_votes

    uint8_t result_status = 2;

    if (bal.unique_voters >= quorum_thresh && bal.yes_count >= pass_thresh) { //proposal passed, refund granted

        proposals.modify(p, 0, [&]( auto& a ) {
            a.status = 1;
        });

        action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer), make_tuple(
            _self,
            prop.proposer,
            asset(int64_t(1000000), S(4, TLOS)),
            std::string("Ratify/Amend Proposal Fee Refund")
        )).send();

        update_doc(prop.document_id, prop.new_clause_nums, prop.new_ipfs_urls);

        result_status = 1;

    } else if (bal.unique_voters >= q_refund_thresh && bal.yes_count >= p_refund_thresh) { //proposal failed, refund granted

        proposals.modify(p, 0, [&]( auto& a ) {
            a.status = 2;
        });

        action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer), make_tuple(
            _self,
            prop.proposer,
            asset(int64_t(1000000), S(4, TLOS)),
            std::string("Ratify/Amend Proposal Fee Refund")
        )).send();
        
    } else { //proposal failed, refund witheld

        proposals.modify(p, 0, [&]( auto& a ) {
            a.status = 2;
        });

        print("\nProposal refund witheld");
    }

    //Inline action to Trail to close vote
    // action(permission_level{ proposer, N(active) }, N(eosio.trail), N(closevote), make_tuple(
    //     prop.proposer,
    //     prop.ballot_id,
    //     result_status
    // )).send();
}

#pragma region Helper_Functions

void ratifyamend::update_doc(uint64_t document_id, vector<uint8_t> new_clause_nums, vector<string> new_ipfs_urls) {
    documents_table documents(_self, _self);
    auto d = documents.find(document_id);
    auto doc = *d;

    auto doc_size = doc.clauses.size();
    for (int i = 0; i < new_clause_nums.size(); i++) {
        
        if (new_clause_nums[i] < doc.clauses.size()) { //update existing clause
            doc.clauses[new_clause_nums[i]] = new_ipfs_urls.at(i);
        } else { //add new clause
            doc.clauses.push_back(new_ipfs_urls.at(i));
        }
    }

    documents.modify(d, 0, [&]( auto& a ) {
        a.clauses = doc.clauses;
    });
}

#pragma endregion Helper_Functions

EOSIO_ABI(ratifyamend, (insertdoc)(makeproposal)(closeprop))
