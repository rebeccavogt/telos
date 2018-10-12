# Trail Walkthrough

Trail offers a comprehensive suite of blockchain-based services.

## Voting and Ballot/Voter Registration

All users on the Telos Blockchain Network can register their account and receive a VoterID card that can be used in any Ballot or Election running on Trail. Developers can easily integrate with Trail by including the `traildef.voting.hpp` plugin in their contracts.

### Voting Actions

* `regvoter()`

    Calling regvoter will affiliate the given account_name with the given token symbol.

* `unregvoter()`

    Calling unregvoter will remove the registration from the table.

* `addreceipt()`

    The addreceipt function is called as an inline action by the external voting contract. This action should only be called when a vote has been deemed valid (criteria for validity is at the discretion of the developer), and the vote is ready to be logged. Once validation passes, the inline action should store the code, scope, and key of the data object for which the vote was cast.

* `rmvexpvotes()`

    The rmvexpreceipts action is called by the owner of a VoterID to remove all expired receipts still logged on their VoterID.

* `regballot()`

    RegBallot is called to register a voting contract.

* `unregballot()`

    Unregballot is called to remove a registered voting contract from the table.

## Token Registration

Developers and organizations can author new and independent token contracts and then register their contract through Trail.

* `regtoken()`
* `unregtoken()`

## Trail Definitions

## Teclos Example Commands