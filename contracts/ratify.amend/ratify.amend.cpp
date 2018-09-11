#include "ratify.amend.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

ratifyamend::ratifyamend(account_name self) : contract(self), thresh(self, self) {
    if (!thresh.exists()) {

        thresh_struct = threshold{
            _self, //publisher
            0, //quorum_threshold
            0, //pass_threshold
            5 //expiration_length
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

void ratifyamend::propose(string title, string ipfs_url, uint64_t document_id, uint64_t clause_id, account_name proposer) {
    require_auth(proposer);

    proposals_table proposals(_self, _self);

    uint64_t prop_id = proposals.available_primary_key();
    uint32_t prop_time = now();

    //TODO: update with proposal struct
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
    });

    print("\nProposal Emplaced");
    print("\nProposal ID: ", prop_id);

}

void ratifyamend::vote(uint64_t proposal_id, uint16_t vote, account_name voter) {
    require_auth(voter);
    eosio_assert(vote >= 0 && vote <= 2, "Invalid Vote. [0 = ABSTAIN, 1 = YES, 2 = NO]");
    

    //-----------VoterID-----------

    voters_table voters(N(trailservice), voter); //TODO: change code to trail.service account
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter Not Found");
    print("\nVoter Found");

    auto vo = *v;

    voteinfo new_vi = voteinfo{
        _self,
        voter,
        proposal_id,
        vote,
        1
    };


    proposals_table proposals(_self, _self);
    auto p = proposals.find(proposal_id);

    eosio_assert(p != proposals.end(), "Proposal Not Found");
    print("\nProposal Found");

    auto po = *p;




    if (vo.votes_list.empty()) {
        print("\nVoteInfo Stack Empty...Inserting Struct");
        vo.votes_list.push_back(new_vi);

        voters.modify(v, 0, [&]( auto& a ) {
            a.votes_list = vo.votes_list;
        });
    } else {
        //search for existing voteinfo with matching proposal id
        //insert new voteinfo struct into vector if not found

        bool found = false;

        for (voteinfo receipt : vo.votes_list) {
            if (receipt.vote_key == proposal_id) {

                print("\nVoterInfo Struct Found");
                found = true;

                //remove existing vote from proposal
                switch (receipt.direction) { //TODO: remove by old weight, not just decrement
                    case 0 : po.abstain_count--;
                    case 1 : po.yes_count--;
                    case 2 : po.no_count--;
                }

                receipt.direction = vote;
                receipt.weight = 1; //TODO: Get true vote weight

                vo.votes_list[proposal_id] = receipt;

                voters.modify(v, 0, [&]( auto& a ) {
                    a.votes_list = vo.votes_list;
                });

                //break; //necessary? may help reduce computation
            }
        }

        if (found == false) {
            vo.votes_list.push_back(new_vi);

            voters.modify(v, 0, [&]( auto& a ) {
                a.votes_list = vo.votes_list;
            });
        }
    }
    

    //TODO: increase by new vote weight, not just increment
    switch (vote) {
        case 0 : po.abstain_count++;
        case 1 : po.yes_count++;
        case 2 : po.no_count++;
    }

    proposals.modify(p, 0, [&]( auto& a ) {
        a.abstain_count = po.abstain_count;
        a.yes_count = po.yes_count;
        a.no_count = po.no_count;
    });

    string vote_type;
    switch (vote) {
        case 0 : vote_type = "ABSTAIN";
        case 1 : vote_type = "YES";
        case 2 : vote_type = "NO";
    }
    print("\nYour Vote: ", vote_type);
}


void ratifyamend::update_thresh() { //NOTE: tentative values

    environment_singleton env(N(trailservice), N(trailservice)); //TODO: change code to trailservice account
    environment e = env.get();

    uint64_t new_quorum = e.total_voters / 4; //25% of all registered voters
    uint64_t new_pass = (new_quorum / 2) + 1; // 50% + 1 of quorum

    thresh_struct.quorum_threshold = new_quorum;
    thresh_struct.pass_threshold = new_pass;

    print("\nEnvironment Thresholds Updated");
}

EOSIO_ABI( ratifyamend, (propose)(vote))