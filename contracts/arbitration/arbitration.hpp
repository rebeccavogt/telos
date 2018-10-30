/**
 * The arbitration contract is used as an interface for the Telos arbitration system. Users register their
 * account through the Trail Service, where they will be issued a VoterID card that tracks and stores vote
 * participation.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#include <../trail.service/trail.connections/trailconn.voting.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class arbitration : public contract {
    public:

        #pragma region Enums

        enum case_state {
            CASE_SETUP, //0
            AWAITING_ARBS, //1
            CASE_INVESTIGATION, //2
            DISMISSED, //3
            HEARING, //4
            DELIBERATION, //5
            DECISION, //6 NOTE: No more joinders allowed
            ENFORCEMENT, //7
            COMPLETE //8
        };

        enum claim_class {
            UNDECIDED, //0
            LOST_KEY_RECOVERY, //1
            TRX_REVERSAL, //2
            EMERGENCY_INTER, //3
            CONTESTED_OWNER, //4
            UNEXECUTED_RELIEF, //5
            CONTRACT_BREACH, //6
            MISUSED_CR_IP, //7
            A_TORT, //8
            BP_PENALTY_REVERSAL, //9
            WRONGFUL_ARB_ACT, //10
            ACT_EXEC_RELIEF, //11
            WP_PROJ_FAILURE, //12
            TBNOA_BREACH, //13
            MISC //14
        };

        //CLARIFY: can arbs determine classes of cases they take? YES
        enum arb_status {
            AVAILABLE, //0
            UNAVAILABLE, //1
            INACTIVE //2
        };

        enum election_status {
            OPEN, //0
            PASSED, //1
            FAILED //2
        };

        //TODO: Evidence states

        #pragma endregion Enums

        #pragma region Structs

        //NOTE: diminishing subsequent response (default) times
        //NOTE: initial deposit saved
        //NOTE: class of claim where neither party can pay fees, TF pays instead
        /// @abi table context i64
        struct config {
            account_name publisher;
            uint16_t max_arbs;
            uint32_t default_time;
            vector<asset> fee_structure;
            //TODO: Arbitrator schedule field based on class
            //CLARIFY: usage of "schedule" in requirements doc

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(config, (publisher)(max_arbs)(default_time))
        };

        /// @abi table elections i64
        struct election {
            account_name candidate;
            string credentials;
            uint32_t yes_votes;
            uint32_t no_votes;
            uint32_t abstain_votes;
            uint32_t expire_time;
            uint16_t election_status;

            uint64_t primary_key() const { return candidate; }
            EOSLIB_SERIALIZE(election, (candidate)(credentials)(yes_votes)(no_votes)(abstain_votes)(expire_time)(election_status))
        };

        /// @abi table arbitrators i64
        struct arbitrator {
            account_name arb;
            uint16_t arb_status;
            vector<uint64_t> open_case_ids;
            vector<uint64_t> closed_case_ids;
            //string credentials; //ipfs_url of credentials
            //vector<string> languages; //NOTE: language codes for space

            uint64_t primary_key() const { return arb; }
            EOSLIB_SERIALIZE(arbitrator, (arb)(arb_status)(open_case_ids)(closed_case_ids))
        };

        struct claim {
            uint16_t class_suggestion;
            vector<string> submitted_pending_evidence; //submitted by claimant
            vector<uint64_t> accepted_ev_ids; //accepted and emplaced by arb
            uint16_t class_decision; //initialized to UNDECIDED (0)

            EOSLIB_SERIALIZE(claim, (class_suggestion)(submitted_pending_evidence)(accepted_ev_ids)(class_decision))
        };

        //TODO: evidence types?
        //NOTE: add metadata
        /// @abi table evidence i64
        struct evidence {
            uint64_t ev_id;
            string ipfs_url;

            uint64_t primary_key() const { return ev_id; }
            EOSLIB_SERIALIZE(evidence, (ev_id)(ipfs_url))
        };

        //NOTE: joinders saved in separate table
        /// @abi table casefiles i64
        struct casefile {
            uint64_t case_id;
            account_name claimant; //TODO: add vector for claimant's party? same for respondant and their party?
            account_name respondant; //NOTE: can be set to 0
            vector<claim> claims;
            vector<account_name> arbitrators; //CLARIFY: do arbitrators get added when joining?
            uint16_t case_status;
            uint32_t last_edit;
            vector<string> findings_ipfs;
            //vector<asset> additional_fees; //NOTE: case by case?
            //TODO: add messages field

            uint64_t primary_key() const { return case_id; }
            uint64_t by_claimant() const { return claimant; }
            EOSLIB_SERIALIZE(casefile, (case_id)(claimant)(claims)(arbitrators)(case_status)(last_edit)(findings_ipfs))
        };

        #pragma endregion Structs

        arbitration(account_name self);

        ~arbitration();

        void setconfig(uint16_t max_arbs, uint32_t default_time, vector<asset> fees); //setting global configuration 

        #pragma region Arb_Elections

        void applyforarb(account_name candidate, string creds_ipfs_url); //TODO: rename to arbapply(), newarbapp()

        void cancelarbapp(account_name candidate); //TODO: rename to arbunapply(), rmvarbapply()

        void voteforarb(account_name candidate, uint16_t direction, account_name voter);

        void endelection(account_name candidate); //automate in constructor?

        #pragma endregion Arb_Elections

        #pragma region Case_Setup

        void filecase(account_name claimant, uint16_t class_suggestion, string ev_ipfs_url); //NOTE: filing a case doesn't require a respondent

        void addclaim(uint64_t case_id, uint16_t class_suggestion, string ev_ipfs_url, account_name claimant); //NOTE: adds subsequent claims to a case

        void removeclaim(uint64_t case_id, uint16_t claim_num, account_name claimant); //NOTE: Claims can only be removed by a claimant during case setup. Enfore that have atleas one claim before awaiting arbs

        void shredcase(uint64_t case_id, account_name claimant); //NOTE: member-level case removal, called during CASE_SETUP

		void readycase(uint64_t case_id, account_name claimant);

        #pragma endregion Case_Setup

        #pragma region Member_Only

        void vetoarb(uint64_t case_id, account_name arb, account_name selector);

        #pragma endregion Member_Only

        #pragma region Arb_Only

		//TODO: Set case respondant action

        void dismisscase(uint64_t case_id, account_name arb, string ipfs_url); //TODO: require rationale?

        void closecase(uint64_t case_id, account_name closer, string ipfs_url); //TODO: require decision?

        void dismissev(uint64_t case_id, uint16_t claim_index, uint16_t ev_index, account_name arb, string ipfs_url); //NOTE: moves to dismissed_evidence table

        void acceptev(uint64_t case_id, uint16_t claim_index, uint16_t ev_index, account_name arb, string ipfs_url); //NOTE: moves to evidence_table and assigns ID

        void arbstatus(uint16_t new_status, account_name arb);

        void casestatus(uint64_t case_id, uint16_t new_status, account_name arb);

        void changeclass(uint64_t case_id, uint16_t claim_index, uint16_t new_class, account_name arb);

        //void joincases(vector<uint64_t> case_ids, account_name arb); //CLARIFY: joined case is rolled into Base case?

        //void addevidence(uint64_t case_id, vector<uint64_t> ipfs_urls, account_name arb); //NOTE: member version is submitev()

        void recuse(uint64_t case_id, string rationale, account_name arb);

        #pragma endregion Arb_Only

        #pragma region BP_Multisig_Actions

        void dismissarb(account_name arb);

        #pragma endregion BP_Multisig_Actions

        //void message(uint64_t case_id, account_name from, account_name to, string msg);

        //TODO: Add evidence action on chain evidence (public?) and off chain evidence (private?)
        //NOTE: off chain evidence is still recorded on the chain as being submitted,
        // and contains meta data about the offchain information submitted
        // zero knowledge proof?

        //TODO: set additional fee structure for a case

    private:

    protected:

        #pragma region Helper_Functions

		void validate_ipfs_url(string ipfs_url);

        bool is_candidate(account_name candidate);

        bool is_arb(account_name arb);

        bool is_case(uint64_t case_id);

        bool is_election_open(account_name candidate);

        bool is_election_expired(account_name candidate);

        //void require_arb(account_name arb);

        #pragma endregion Helper_Functions

        #pragma region Tables

        typedef singleton<N(configs), config> config_singleton;
        config_singleton configs;
        config _config;
 
        typedef multi_index<N(elections), election> elections_table;
        typedef multi_index<N(arbitrators), arbitrator> arbitrators_table;

        typedef multi_index<N(casefiles), casefile> casefiles_table;
        typedef multi_index<N(dismisscases), casefile> dismissed_cases_table;

        typedef multi_index<N(evidence), evidence> evidence_table;
        typedef multi_index<N(dismissedev), evidence> dismissed_evidence_table;

        #pragma endregion Tables
};