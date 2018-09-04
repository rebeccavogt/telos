# CONTRACT FOR registry::transfer

## ACTION NAME: transfer

### Parameters
Input paramters:

* `owner` (account_name transferring tokens.)

* `recipient` (account_name receiving transferred tokens.)

* `tokens` (asset to be transferred.)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to transfer an asset from one account to another. The recipient account must have an existing wallet in the native contract in order to receive tokens, otherwise the action will fail by assertion. The sent funds are immediatley subtracted from the `owner`'s wallet entry and added to the `recipient`'s wallet entry.

### Term
TERM. This Contract expires at the conclusion of code execution.