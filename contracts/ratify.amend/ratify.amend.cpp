/**
 * 
 * 
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/singleton.hpp>

#include "ratify.amend.hpp"

ratifyamend::ratifyamend(account_name self) : contract(self), oa(self, self) {
    if (!oa.exists()) {

        _oa = document{
            self,
            permission_name{uint64_t(5)}
        };

        oa.set(_oa, self);
        _oa = oa.get();
    } else {

        _oa = oa.get();
    }
}

ratifyamend::~ratifyamend() {
    if (oa.exists()) {
        oa.set(_oa, _oa.last_edit_made_by);
    }
}

void getpermission(account_name actor, permission_name permission) {

}

void sendproposal(account_name proposer, name proposal_name, vector<permission_level> requested, transaction trx) {
    //INLINE_ACTION_SENDER(multisig, propose)
        //(N(eosio.msig), {N(eosio.msig), N(active)},
        //{ proposer, proposal_name, requested, trx});
}