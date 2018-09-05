# CONTRACT FOR registry::transferfrom

## ACTION NAME: transferfrom

### Parameters
Input parameters:

* `owner` (account_name from which alloted tokens are transferred.)

* `recipient` (account_name receiving transferred tokens.)

* `tokens` (asset to be transferred.)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to allow the transfer of tokens from the owner allotment to the recipient account. The recipient account must have an existing wallet in the native contract in order to receive tokens, otherwise the action will fail by assertion. The sent funds are immediately subtracted from the `owner`'s wallet entry and added to the `recipient`'s wallet entry.

### Term
TERM. This Contract expires at the conclusion of code execution.