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

        enum class_type {
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
            UNAVAILABLE //1
        };

        arbitration(account_name self);

        ~arbitration();

        void setconfig(); //setting global global configuration 

        #pragma region Member_Actions

        void filecase(); //NOTE: filing a case doesn't require a respondent

        void closecase();

        void message();

        // TODO: Add evidence action on chain evidence (public?) and off chain evidence (private?)
        // exhibits... possibly multiple actions
        // NOTE: off chain evidence is still recorded on the chain as being submitted,
        // and contains meta data about the offchain information submitted
        // zero knowledge proof?

        void regarb();

        void unregarb();

        void votearb();

        void selectarbs();
            
        #pragma endregion Member_Actions

        #pragma region Arb_Actions

        void addevidence();

        void editarbstat();

        void editcasestat();

        void joincases(); //CLARIFY: joined case is rolled into Base case?

        void changeclass();

        void removeclaim();

        void droparb(); //NOTE: called by multi-sig to drop arbitrator from case

        void dismissarb(); //NOTE: called by multi-sig to force unregarb()

        void recuse(); //CLARIFY: provide written explanation of reasoning for recusal

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
            vector<asset> fee_structure;
            //TODO: fee schedule?
            //uint32_t default_time_limit_us; //TODO: double check time_point units
            //CLARIFY: usage of "schedule" in requirements doc

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(config, (publisher)(fee_structure))
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

        struct claim {
            uint64_t claim_id;
            class_type type;
            vector<string> ipfs_urls;

            //uint64_t primary_key() const { return claim_id; }
            //EOSLIB_SERIALIZE(claim, (claim_id)(type))
        };

        //NOTE: joinders saved in separate table
        /// @abi table casefiles i64
        struct casefile {
            uint64_t case_id;
            vector<uint64_t> evidence_ids;
            vector<account_name> arbitrators; //CLARIFY: do arbitrators get added when joining?
            vector<claim> claims;
            case_status status;
            block_timestamp last_edit;
            //vector<asset> additional_fees; //NOTE: case by case?
            //TODO: add messages field

            uint64_t primary_key() const { return case_id; }
            EOSLIB_SERIALIZE(casefile, (case_id)(evidence_id))
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

        typedef singleton<N(config), config> config_singleton;

        typedef multi_index<N(arbitrators), arbitrator> arbitrators_table;
        typedef multi_index<N(casefiles), casefile> casefiles_table;
        typedef multi_index<N(evidence), evidence> evidence_table;
};