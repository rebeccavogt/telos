# CONTRACT FOR registry::createwallet

## ACTION NAME: createwallet

### Parameters
Input parameters:

* `owner` (account_name calling the action to create a balance entry.)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to allow the creation of a zero balance entry in the balances table for the calling account. Users should call this action first to create a balance entry for holding their future tokens. This will consume a small amount of RAM, which will be reclaimed should the user ever choose to delete their balance entry.

### Term
TERM. This Contract expires at the conclusion of code execution.