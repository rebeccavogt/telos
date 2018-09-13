#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class ratifyamend : public contract {
    public:

        ratifyamend(account_name self);

        ~ratifyamend();

        // ABI Actions
        void insertdoc(string title, vector<string> clauses);

        void propose(string title, string ipfs_url, uint64_t document_id, uint64_t clause_id, account_name proposer);

        void vote(uint64_t proposal_id, uint16_t vote, account_name voter);

        void close(uint64_t proposal_id);

    protected:

        void update_thresh();

        /// @abi table documents i64
        struct document {
            uint64_t id;
            string title;
            vector<string> clauses; //vector of ipfs urls

            uint64_t primary_key() const { return id; }
            EOSLIB_SERIALIZE(document, (id)(title)(clauses))
        };

        /// @abi table proposals i64
        struct proposal { //TODO: implement expiration
            uint64_t id;
            uint64_t document_id;
            uint64_t clause_id;
            string title;
            string ipfs_url;
            uint64_t yes_count;
            uint64_t no_count;
            uint64_t abstain_count;
            account_name proposer;
            uint32_t expiration;
            string status; // "OPEN", "PASSED", "FAILED"

            uint64_t primary_key() const { return id; }
            EOSLIB_SERIALIZE(proposal, (id)(document_id)(clause_id)(title)(ipfs_url)(yes_count)(no_count)(abstain_count)(proposer)(expiration)(status))
        };

        /// @abi table threshold
        struct threshold {
            account_name publisher;
            uint64_t quorum_threshold;
            uint32_t expiration_length;

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(threshold, (publisher)(quorum_threshold)(expiration_length))
        };

    typedef multi_index<N(documents), document> documents_table;
    typedef multi_index<N(proposals), proposal> proposals_table;

    typedef singleton<N(threshold), threshold> threshold_singleton;
    threshold_singleton thresh;
    threshold thresh_struct;

    //---------------------Definitions from trailservice---------------------

    struct voteinfo {
        uint64_t vote_code; // code of contract receiving vote
        uint64_t vote_scope; // scope of contract receiving vote
        uint64_t vote_key; // key to retrieve voted object
        uint16_t direction; // 0 = abstain, 1 = yes, 2 = no TODO: use enum? 
        uint64_t weight; // weight of votes applied
    };

    struct voterid {
        account_name voter;
        vector<voteinfo> votes_list;
        uint64_t tlos_weight;

        uint64_t primary_key() const { return voter; }
        EOSLIB_SERIALIZE(voterid, (voter)(votes_list)(tlos_weight))
    };

    struct environment {
        account_name publisher;

        uint64_t total_tokens;
        uint64_t total_voters;

        uint64_t primary_key() const { return publisher; }
        EOSLIB_SERIALIZE(environment, (publisher)(total_tokens)(total_voters))
    };

    typedef multi_index<N(voters), voterid> voters_table;
    typedef singleton<N(environment), environment> environment_singleton;
};