# CONTRACT FOR registry::reclaim

## ACTION NAME: reclaim

### Parameters
Input parameters:

* `owner` (account_name receiving reclaimed tokens.)

* `recipient` (account_name returning reclaimed tokens.)

* `tokens` (asset to be reclaimed.)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to allow the allotment of reclaimed tokens back into the owner's balance. The owner account must have an existing wallet in the native contract in order to receive tokens, otherwise the action will fail by assertion. The sent funds are immediately subtracted from the `recipient`'s wallet entry and added to the `owner`'s wallet entry.

### Term
TERM. This Contract expires at the conclusion of code execution.