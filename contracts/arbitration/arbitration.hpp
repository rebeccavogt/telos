/**
 * The arbitration contract is used as an interface for the Telos arbitration system. Users register their
 * account through the Trail Service, where they will be issued a VoterID card that tracks and stores vote
 * participation.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva
 * @copyright defined in telos/LICENSE.txt
 */

#include <../trail.service/trail.connections/trailconn.voting.hpp> //trailservice voting data definitions

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
            FILED, //0
            AWAITING_ARBS, //1
            CASE_INVESTIGATION, //2
            DISMISSED, //3
            HEARING, //4
            DELIBERATION, //5
            DECISION, //6
            ENFORCED //7
        };

        enum claim_type {
            TRX_REVERSAL, //0
            EMERGENCY_INTER, //1
            CONTESTED_OWNER, //2
            UNEXECUTED_RELIEF, //3
            CONTRACT_BREACH, //4
            MISUSED_CR_IP, //5
            A_TORT, //6
            BP_PENALTY_REVERSAL, //7
            WRONGFUL_ARB_ACT, //8
            ACT_EXEC_RELIEF, //9
            WP_PROJ_FAILURE, //10
            TBNOA_BREACH, //11
            MISC //12
        };

        enum class_type {
            
        };

        enum arb_status {
            AVAILABLE, //0
            UNAVAILABLE //1
        };

        arbitration(account_name self);

        ~arbitration();

        void setcontext();

        #pragma region Member_Actions

        void filecase();

        void closecase();

        void message();

        void regarb();

        void unregarb();

        void votearb();

        void selectarbs();
            
        #pragma endregion Member_Actions

        #pragma region Arb_Actions

        void addevidence();

        void editarbstat();

        void editcasestat();

        void joincases(); //NOTE: joined case is rolled into Base case

        void changeclass();

        void droparb(); //NOTE: called by multi-sig to drop arbitrator from case

        void dismissarb(); //NOTE: called by multi-sig to force unregarb()

        void recuse();

        //void removeclaim();

        //TODO: set additional fee structure for a case

        #pragma endregion Arb_Actions
    private:

    protected:

        /// @abi table context i64
        struct context {
            account_name publisher;
            uint16_t max_concurrent_arbs;
            //TODO: Arbitrator schedule field based on class
            vector<asset> fee_structure;
            //TODO: fee schedule?
            //uint32_t default_time_limit_us; //TODO: double check time_point units
            //CLARIFY: usage of "schedule" in requirements doc

            uint64_t primary_key() const { return publisher; }
            EOSLIB_SERIALIZE(context, (publisher))
        };

        /// @abi table arbitrators i64
        struct arbitrator {
            account_name arb;
            vector<uint64_t> case_ids;
            arb_status status;

            uint64_t primary_key() const { return arb; }
            EOSLIB_SERIALIZE(arbitrator, (arb))
        };

        struct claim {
            uint64_t claim_id;
            claim_type type;
            vector<string> ipfs_urls;

            //uint64_t primary_key() const { return claim_id; }
            //EOSLIB_SERIALIZE(claim, (claim_id)(type))
        };

        /// @abi table casefiles i64
        struct casefile {
            uint64_t case_id;
            vector<uint64_t> evidence_ids;
            vector<account_name> arbitrators; //CLARIFY: do arbitrators get added when joining?
            vector<claim> claims;
            case_status status;
            block_timestamp last_edit;
            //vector<asset> additional_fees; //NOTE: case by case?

            uint64_t primary_key() const { return case_id; }
            EOSLIB_SERIALIZE(casefile, (case_id)(evidence_id))
        };

        /// @abi table evidence i64
        struct evidence {
            uint64_t ev_id;
            vector<string> ipfs_urls;

            uint64_t primary_key() const { return ev_id; }
            EOSLIB_SERIALIZE(evidence, (ev_id)(ipfs_urls))
        };

        typedef singleton<N(context), context> context_singleton;

        typedef multi_index<N(arbitrators), arbitrator> arbitrators_table;
        typedef multi_index<N(casefiles), casefile> casefiles_table;
        typedef multi_index<N(evidence), evidence> evidence_table;
};