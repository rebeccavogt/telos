# TIP-5 Interface and Sample Implementation Overview

For a complete code walkthrough, go [here]().

### Tables and Structs

The TIP-5 standard uses three tables to store information about token balances, allotments, and global token settings.

* `balances_table` The balances table is used to store the native token balances of each user on the contract.

    The balances_table typedef is defined in the interface, allowing developers to intantiate the table with any code or scope as needed. The balances table stores `balance` structs, indexed by the `owner` member.

    * `owner` is the account that owns the token balance.
    * `tokens` is the native asset, where `tokens.amount` is the quantity held.

* `allotments_table` The allotments_table is a table separate from balances that allows users to make allotments to a desired recipient. The recipient can then collect their allotment at any time, provided the user who made the allotment has not reclaimed it.

    The allotments_table typedef is defined in the interface, allowing developers to instantiate the table with any code or scope as needed. The allotments table stores `allotment` structs, indexed by the `recipient` member.

    * `recipient` is the account that is intended to receive the allotment.
    * `owner` is the account that made the allotment.
    * `tokens` is the asset allotted, where `tokens.amount` is the quantity allotted.

* `_settings` The _settings instance of the `settings_table` typedef is a global singleton that is insantiated and set automatically by the constructor and destructor, respectively. 

    The _settings singleton holds all information about the native token managed by the contract. This information can be set by creating an `order` struct with the desired settings and then setting that struct into the singleton at contract destruction. This design pattern allows the _settings singleton to be modified at will during execution, and then the final state will be saved automatically when execution ends. See the constructor and destructor implementation for an example.

    * `issuer` is the account that owns the token contract and has the authority to mint new tokens.
    * `max_supply` is the maximum amount of tokens that can be in circulation at any given time.
    * `supply` is the amount of tokens currently in circulation.
    * `name` is the human-readable name of the native token.

### Actions

The following is a walkthrough of the TIP-5 Token Registry Standard actions:

* `mint` Mint is called to create tokens, thereby introducing new tokens into circulation.
  
* `transfer` Transfer is called to send tokens from one account to another.

* `allot` Allot is called to make an allotment to another account.

* `reclaim` Reclaim is called to cancel an allotment and return the allotted tokens to the owner's wallet.

* `transferfrom` TransferFrom is called by the intended recipient to collect an allotment.

* `createwallet` CreateWallet is called to create a zero-balance entry in the balances table so the user can receive tokens freely.

    While it is not strictly necessary to have a function such as `createwallet`, it's needed in order to avoid having ram "leaks" plague contract users. A ram "leak" occurs when an account uses some of it's own ram to store data for another account, and while it's a relatively small amount of ram (at least in the TIP-5 implementation), it has the potential to add up and result in a significant drain of resources. This rings double-true when practices like ram speculation are used to drive up prices, making the cost of development even steeper.
    
    For example, Account A wants to transfer 1.00 TEST token to Account B, but Account B doesn't have a wallet created on the TEST token contract (in other words, Account B doesn't own any TEST token, not even an amount of zero). If Account A were to transfer the TEST token anyway, it would need to first pay ram out of it's own pocket to cover the cost of creating the balance entry before emplacing the transfer (this is disabled in the TIP-5 implementation by default). While it is possible to give the ram back to Account A through making a subsequent table modification and passing Account B as the ram payer, it is much better (and cleaner) practice to enforce users to pay for their own ram requirements outright.

* `deletewallet` DeleteWallet is called to delete a zero-balance entry in the balances table.

    It is critical to ensure a wallet entry is only deleted if it has a balance of exactly zero. Deleting wallets with balances in them is the equivalent of burning those tokens and not removing them from the maximum supply. Future TIP extentions will introduce token burning features that account for this as well as a variable max supply.










### Setup Master Registry
teclos set contract eosio.token ./ eosio.token.wasm eosio.token.abi

### Setup Telex A
teclos set contract prodname1 ./ telex.sample.wasm telex.sample.abi
teclos push action prodname1 createwallet '{"owner": "prodname1"}' -p prodname1@active
teclos push action prodname2 createwallet '{"owner": "prodname2"}' -p prodname2@active
teclos push action prodname1 mint '{"recipient": "prodname1", "tokens": "10.00 TTT"}' -p prodname1@active
teclos push action eosio.token subscribe '{"publisher": "prodname1", "native": "0.00 TTT"}' -p prodname1@active

### Setup Telex B
teclos set contract prodname2 ./ telex.sample2.wasm telex.sample2.abi
teclos push action prodname2 createwallet '{"owner": "prodname1"}' -p prodname1@active
teclos push action prodname2 createwallet '{"owner": "prodname2"}' -p prodname2@active
teclos push action prodname2 mint '{"recipient": "prodname2", "tokens": "10.00 UUU"}' -p prodname2@active
teclos push action eosio.token subscribe '{"publisher": "prodname2", "native": "0.00 UUU"}' -p prodname2@active

### Testing
teclos push action prodname2 buy '{"native": "1.00 TTT", "held": "1.00 UUU", "buyer": "prodname2"}' -p prodname2@active
teclos push action prodname1 sell '{"held": "1.00 TTT", "desired": "1.00 UUU", "seller": "prodname1"}' -p prodname1@active