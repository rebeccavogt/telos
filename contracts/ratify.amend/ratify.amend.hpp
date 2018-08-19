#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class ratifyamend : public eosio::contract {
    public:

        ratifyamend(account_name self);

        ~ratifyamend();

        void getpermission(account_name actor, permission_name permission);

        void sendproposal(account_name proposer, name proposal_name, vector<permission_level> requested, transaction trx);

    private:

    protected:

        struct document {
            account_name last_edit_made_by;
            permission_name p;
        };

    typedef eosio::singleton<N(opag), document> operating_agreement;
    operating_agreement oa;
    document _oa;
};