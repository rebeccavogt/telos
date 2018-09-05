#CONTRACT FOR registry::mint

## ACTION NAME: mint

### Parameters
Input paramters:

* `recipient` (account_name receiving minted tokens.)

* `tokens` (asset to be minted)

Implied parameters: 

* `account_name` (name of the party invoking and signing the contract)

### Intent
INTENT. The intention of the author and the invoker of this contract is to mint new tokens into circulation. The recipient party will receive the newly minted tokens, provided they have a wallet created under the code of this contract. Minting tokens to an account_name without a wallet will fail by assertion. Only the account_name defined as the `issuer` in the `settings` singleton has the authority to mint new tokens.

### Term
TERM. This Contract expires at the conclusion of code execution.