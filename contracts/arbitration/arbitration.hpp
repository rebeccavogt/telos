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

        //======================= Enums =========================

        enum case_status {
            AWAITING_ARBS, //0
            CASE_INVESTIGATION, //1
            DISMISSED, //2
            HEARING, //3
            DELIBERATION, //4
            DECISION, //5 NOTE: No more joinders allowed
            ENFORCEMENT, //6
            COMPLETE //7
        };

        enum claim_class {
            LOST_KEY_RECOVERY, //0
            TRX_REVERSAL, //1
            EMERGENCY_INTER, //2
            CONTESTED_OWNER, //3
            UNEXECUTED_RELIEF, //4
            CONTRACT_BREACH, //5
            MISUSED_CR_IP, //6
            A_TORT, //7
            BP_PENALTY_REVERSAL, //8
            WRONGFUL_ARB_ACT, //9
            ACT_EXEC_RELIEF, //10
            WP_PROJ_FAILURE, //11
            TBNOA_BREACH, //12
            MISC //13
        };

        //TODO: Evidence states

        //CLARIFY: can arbs determine classes of cases they take? YES
        //NOTE: additional states: ineligible (possibly the same as is_active) or (not meeting min requirements)
        enum arb_status {
            AVAILABLE, //0
            UNAVAILABLE, //1
            INELIGIBLE //2
        };

        //======================= Structs =========================

        struct claim {
            claim_class class_suggestion;
            claim_class class_decision;
            vector<string> ipfs_urls;

            //uint64_t primary_key() const { return claim_id; }
            //EOSLIB_SERIALIZE(claim, (class_suggestion)(class_decision)(ipfs_urls))
        };

        /// @abi table arbitrators i64
        struct arbitrator {
            account_name arb;
            vector<uint64_t> case_ids;
            arb_status status;
            bool is_active;
            vector<string> languages; //NOTE: language codes (enum?)

            uint64_t primary_key() const { return arb; }
            EOSLIB_SERIALIZE(arbitrator, (arb)(is_active))
        };

        //TODO: evidence types?
        //NOTE: add metadata
        /// @abi table evidence i64
        struct evidence {
            uint64_t ev_id;
            vector<string> ipfs_urls;

            uint64_t primary_key() const { return ev_id; }
            EOSLIB_SERIALIZE(evidence, (ev_id)(ipfs_urls))
        };

        //NOTE: joinders saved in separate table
        /// @abi table casefiles i64
        struct casefile {
            uint64_t case_id;
            vector<uint64_t> pending_evidence_ids;
            vector<uint64_t> dismissed_evidence_ids;
            vector<uint64_t> accepted_evidence_ids;
            vector<account_name> arbitrators; //CLARIFY: do arbitrators get added when joining?
            vector<claim> claims;
            case_status status;
            block_timestamp last_edit;
            //vector<asset> additional_fees; //NOTE: case by case?
            //TODO: add messages field

            uint64_t primary_key() const { return case_id; }
            EOSLIB_SERIALIZE(casefile, (case_id)(evidence_id))
        };

        //======================= Actions =========================

        arbitration(account_name self);

        ~arbitration();

        void setconfig(uint16_t max_arbs, uint32_t default_time); //setting global configuration 

        #pragma region Member_Actions

        void filecase(account_name claimant, vector<claim> claims); //NOTE: filing a case doesn't require a respondent

        void closecase(uint64_t case_id); //CLARIFY: should anyone be able to close case? accusations are serious, potential greifing tool

        void message(account_name from, account_name to, string msg);

        // TODO: Add evidence action on chain evidence (public?) and off chain evidence (private?)
        // exhibits... possibly multiple actions
        // NOTE: off chain evidence is still recorded on the chain as being submitted,
        // and contains meta data about the offchain information submitted
        // zero knowledge proof?

        void regarb(account_name candidate);

        void unregarb(account_name arb);

        void votearb(account_name candidate, uint16_t direction, account_name voter);

        void selectarbs(vector<account_name> arbs, account_name selector);
            
        #pragma endregion Member_Actions

        #pragma region Arb_Actions

        void addevidence(uint64_t case_id, vector<uint64_t> ipfs_urls, account_name arb);

        void editarbstat(account_name arb, arb_status new_status);

        void editcasestat(uint64_t case_id, case_status new_status, account_name arb);

        void joincases(vector<uint64_t> case_ids, account_name arb); //CLARIFY: joined case is rolled into Base case?

        void changeclass(uint64_t case_id, uint16_t claim_key, account_name arb);

        void removeclaim(uint64_t case_id, uint16_t claim_key, account_name arb);

        void droparb(account_name arb); //NOTE: called by multi-sig to drop arbitrator from case

        void dismissarb(account_name arb); //NOTE: called by multi-sig to force unregarb()

        void recuse(uint64_t case_id, account_name arb); //CLARIFY: provide written explanation of reasoning for recusal

        //TODO: set additional fee structure for a case

        #pragma endregion Arb_Actions
    private:

    protected:

        //NOTE: initial respondent response times different than subsequent response times
        //NOTE: initial deposit saved
        //NOTE: class of claim where neither party can pay fees, TF pays instead
        /// @abi table context i64
        struct config {
            account_name publisher;
            uint16_t max_concurrent_arbs;
            //TODO: Arbitrator schedule field based on class
            //vector<asset> fee_structure;
            //TODO: fee schedule?
            uint32_t default_time_limit; //TODO: double check time_point units
            //CLARIFY: usage of "schedule" in requirements doc

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(config, (publisher)(fee_structure))
        };

         //======================= Tables =========================

        typedef singleton<N(config), config> config_singleton;

        typedef multi_index<N(candidates), arbitrator> candidates_table;
        typedef multi_index<N(arbitrators), arbitrator> arbitrators_table;

        typedef multi_index<N(casefiles), casefile> casefiles_table;

        typedef multi_index<N(pendingev), evidence> pendingevidence_table;
        typedef multi_index<N(evidence), evidence> evidence_table;
};