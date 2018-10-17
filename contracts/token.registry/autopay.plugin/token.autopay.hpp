/**
 * 
 */
#include "../token.registry.hpp"

namespace registry {

    /// @abi table payments
    struct payment {
        uint64_t payment_id;
        account_name recipient;
        account_name payer;
        asset amount;

        uint64_t primary_key() const { return payment_id; }
        uint64_t by_recipient() const { return recipient; }
        uint64_t by_payer() const { return payer; }
        EOSLIB_SERIALIZE(payment, (payment_id)(recipient)(payer)(amount))
    };

    typedef multi_index<N(payments), payment,
        indexed_by<N(recipient), const_mem_fun<payment, uint64_t, &payment::by_recipient>>,
        indexed_by<N(payer), const_mem_fun<payment, uint64_t, &payment::by_payer>>> payments_table;
    
    void autopay(account_name payer, account_name recipient, asset tokens, uint32_t period);

    void rmvautopay(); //TODO: rename. rmvautopay is stupid
}