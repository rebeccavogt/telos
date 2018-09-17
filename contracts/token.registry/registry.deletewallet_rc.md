# CONTRACT FOR registry::deletewallet

## ACTION NAME: deletewallet

### Parameters
Input parameters:

* `owner` (account_name calling the action to delete an entry in the balances table.

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to allow the deletion of an entry in the balances table for the calling account. Ensure the entry has a balance of zero, otherwise tokens will be deleted.


### Term
TERM. This Contract expires at the conclusion of code execution.