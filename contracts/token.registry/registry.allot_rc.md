#CONTRACT FOR registry::allot

## ACTION NAME: allot

### Parameters
Input parameters:

* `owner` (account_name alloting tokens.)

* `recipient` (account_name able to claim allotted tokens.)

* `tokens` (asset to be allotted.)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to allow the recipient to withdraw from the owner account, multiple times, up to the value of tokens. Subsequent allotments called by the same owner to the same recipient are added together. The recipient account must have an existing wallet in the native contract in order to receive tokens, otherwise the action will fail by assertion. The sent funds are immediately subtracted from the `owner`'s wallet entry and added to the `recipient`'s wallet entry.

### Term
TERM. This Contract expires at the conclusion of code execution.