#include "ratify.amend.hpp"

#include <eosiolib/print.hpp>

ratifyamend::ratifyamend(account_name self) : contract(self), thresh_singleton(self, self) {
    if (!thresh_singleton.exists()) {

        thresh_struct = threshold{
            _self, //publisher
            0, //initial total_voters
            0, //initial quorum_threshold
            uint32_t(5000) //expiration_length in seconds (default is 5,000,000 or ~58 days)
        };

        update_thresh();
        thresh_singleton.set(thresh_struct, _self);
    } else {

        thresh_struct = thresh_singleton.get();     
        update_thresh();
        thresh_singleton.set(thresh_struct, _self);
    }
}

ratifyamend::~ratifyamend() {
    if (thresh_singleton.exists()) {
        thresh_singleton.set(thresh_struct, _self);
    }
}

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

void ratifyamend::propose(string title, uint64_t document_id, vector<uint16_t> new_clause_ids, vector<string> new_ipfs_urls, account_name proposer) {
    require_auth(proposer);

    documents_table documents(_self, _self);
    auto d = documents.find(document_id);

    eosio_assert(d != documents.end(), "Document Not Found");
    auto doc_struct = *d;

    eosio_assert(new_clause_ids.size() == new_ipfs_urls.size(), "Clause ID vector and IPFS url vector sizes must match");

    auto doc_size = doc_struct.clauses.size();
    int16_t last_clause_id = -1;
    auto last_ipfs_url = 0;

    for (int i = 0; i < new_clause_ids.size(); i++) {
        eosio_assert(new_clause_ids.at(i) > last_clause_id, "Clause IDs Must Be In Ascending Order");
        last_clause_id = new_clause_ids.at(i);
        eosio_assert(new_clause_ids.at(i) <= (doc_size + 1), "Invalid Clause Number");
        
        last_ipfs_url++;
        if (new_clause_ids.at(i) == (doc_size + 1)) { //if clause is new, increase doc_size
            doc_size++;
        }
    }

    //NOTE: 100.0000 TLOS fee, refunded if proposal passes
    action(permission_level{ proposer, N(active) }, N(eosio.token), N(transfer), make_tuple( //NOTE: susceptible to ram-drain bug
    	proposer,
        _self,
        asset(int64_t(1000000), S(4, TLOS)),
        std::string("Ratify/Amend Proposal Fee")
	)).send();

    proposals_table proposals(_self, _self);

    proposals.emplace(proposer, [&]( auto& a ){
        a.id = proposals.available_primary_key();
        a.document_id = document_id;
        a.title = title;
        a.new_clause_ids = new_clause_ids;
        a.new_ipfs_urls = new_ipfs_urls;
        a.yes_count = asset(0);
        a.no_count = asset(0);
        a.abstain_count = asset(0);
        a.proposer = proposer;
        a.vote_code = _self;
        a.vote_scope = _self;
        a.expiration = now() + thresh_struct.expiration_length;
        a.status = 0;
    });
}

void ratifyamend::vote(uint64_t vote_code, uint64_t vote_scope, uint64_t proposal_id, uint16_t direction, uint32_t expiration, account_name voter) {
    require_auth(voter);
    eosio_assert(direction >= 0 && direction <= 2, "Invalid Vote. [0 = NO, 1 = YES, 2 = ABSTAIN]");

    //TODO: check that voter is registered

    proposals_table proposals(_self, _self); //TODO: change _self to vote_code and vote_scope?
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    auto prop = *p;
    eosio_assert(prop.expiration >= now(), "Proposal Has Expired");

    //TODO: more checks before sending to trail eg. check matching expire

    require_recipient(N(eosio.trail));
    print("\nVote sent to Trail");
}

void ratifyamend::processvotes(uint64_t vote_code, uint64_t vote_scope, uint64_t proposal_id) {
    proposals_table proposals(_self, _self); //TODO: change _self to vote_code and vote_scope?
    auto p = proposals.find(proposal_id);
    eosio_assert(p != proposals.end(), "Proposal Not Found");
    auto prop = *p;
    eosio_assert(prop.expiration < now(), "Proposal is still open");
    
    receipts_table votereceipts(N(eosio.trail), N(eosio.trail));
    auto by_code = votereceipts.get_index<N(bycode)>();
    auto itr = by_code.lower_bound(vote_code);

    print("\neosio.amend processing votes...");

    if (itr == by_code.end()) {
        print("\nno votes to process");
    } else {
        uint64_t loops = 0;
        int64_t new_no_votes = 0;
        int64_t new_yes_votes = 0;
        int64_t new_abs_votes = 0;

        while(itr->vote_code == vote_code && loops < 10) { //loops variable to limit cpu/net expense per call
            
            if (itr->vote_scope == vote_scope &&
                itr->prop_id == proposal_id &&
                now() > itr->expiration) {

                print("\nvr found...counting...");
                
                switch (itr->direction) {
                    case 0 : new_no_votes += itr->weight.amount; break;
                    case 1 : new_yes_votes += itr->weight.amount; break;
                    case 2 : new_abs_votes += itr->weight.amount; break;
                }

                loops++;
            }
            itr++;

        }

        proposals.modify(p, 0, [&]( auto& a ) {
            a.no_count += asset(new_no_votes);
            a.yes_count += asset(new_yes_votes);
            a.abstain_count += asset(new_abs_votes);
        });

        print("\nloops processed: ", loops);
        print("\nnew no votes: ", asset(new_no_votes));
        print("\nnew yes votes: ", asset(new_yes_votes));
        print("\nnew abstain votes: ", asset(new_abs_votes));

        require_recipient(N(eosio.trail));
    }
}

void ratifyamend::close(uint64_t proposal_id) {
    //TODO: implement
}

void ratifyamend::update_thresh() {

    environment_singleton env(N(eosio.trail), N(eosio.trail));
    environment e = env.get();

    uint64_t new_quorum = e.total_voters / 4; //25% of all registered voters
    uint64_t new_total_voters = e.total_voters;

    thresh_struct.quorum_threshold = new_quorum;
    thresh_struct.total_voters = new_total_voters;
}

void ratifyamend::update_doc(uint64_t document_id, vector<uint16_t> new_clause_ids, vector<string> new_ipfs_urls) {
    documents_table documents(_self, _self);
    auto d = documents.find(document_id);
    auto doc = *d;

    auto doc_size = doc.clauses.size();
    for (int i = 0; i < new_clause_ids.size(); i++) {
        
        if (new_clause_ids[i] < doc.clauses.size()) { //update existing clause
            doc.clauses[new_clause_ids[i]] = new_ipfs_urls.at(i);
        } else { //add new clause
            doc.clauses.push_back(new_ipfs_urls.at(i));
        }
    }

    documents.modify(d, 0, [&]( auto& a ) {
        a.clauses = doc.clauses;
    });
}

//EOSIO_ABI(ratifyamend, (insertdoc)(propose)(vote)(close))

extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {
        ratifyamend _ratifyamend(self);
        if(code == self && action == N(insertdoc)) {
            execute_action(&_ratifyamend, &ratifyamend::insertdoc);
        } else if (code == self && action == N(propose)) {
            execute_action(&_ratifyamend, &ratifyamend::propose);
        } else if (code==self && action==N(vote)) {
            execute_action(&_ratifyamend, &ratifyamend::vote);
        } else if (code == self && action == N(close)) {
            execute_action(&_ratifyamend, &ratifyamend::close);
        }
    }
};
