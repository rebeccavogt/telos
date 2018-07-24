# TELOS - A Smart Contract Platform for Everyone

Welcome to the official Telos repository! This software enables anyone to rapidly build and deploy high-performance and high-security blockchain-based applications.

Telos offers a suite of features including:

1. Economic disparity solutions such as elimination of whale accounts, inversely weighted voting, and increased token equity.
2. Developer and Enterprise features such as designation of propietary code and reduced RAM speculation.
3. Network freeze mitigation by regularly rotating standby BP's into production status to ensure operability and uptime.
4. Standby and BP payments are straightforward and fair, resulting in a smoother payment experience.
5. The Telos Constitution is ratified and enforceable at launch.
6. ...more features incoming!

For a full explanation of all the proposed Telos features, be sure to read our whitepaper!

## Getting Started

1. Clone the most recent changes from our “developer” branch.

2. Open a terminal and navigate to where you cloned the source. Run the below commands.

    a. ./eosio_build.sh -s TLOS

    b. cd build

    c. sudo make install

3. Now setup your nodeos config.ini file.

    a. Determine your Producer name. This is what your node will be identified by on the testnet. You’ll notice we have a naming convention of using ancient philosphers, feel free to follow this convention or come up with your own name. The only rules are the name must be exactly 12 characters and have no special or uppercase characters. Enter your Producer name into the producer-name field in the config.ini file

    b. Determine your Signature Provider. In order to generate a new public and private key pair, you must run “teclos create key” after building the project. Copy your new keys into the signature-provider field in the config.ini file. Don’t forget to run “teclos wallet import your-private-key” to import your private key into your telos wallet.

    c. Configure your p2p endpoints and addresses in the config.ini file. The fields to be changed are http-server-address, p2p-listen-endpoint, p2p-server-address, and p2p-peer-address.

    d. The only plugin required to become a producer is the eosio::producer_plugin, but if you’d like to be able to process transactions or respond to API requests then include the following plugins:

        i. plugin = eosio::http_plugin

        ii. plugin = eosio::chain_plugin

        iii. plugin = eosio::chain_api_plugin

        iv. plugin = eosio::history_plugin

        v. plugin = eosio::history_api_plugin

        vi. plugin = eosio::net_plugin

        vii. plugin = eosio::net_api_plugin

        viii. plugin = eosio::producer_plugin

4. Navigate to testnet.telosfoundation.io and click on the register tab. Enter your configuration into the form and click submit. This will create your account on the testnet. Copy the command generated from the form, you will need it in the following step.

    a. Run the ‘regproducer’ command generated from the previous step in your terminal to register your node on the network. You MUST run this command in order to be added as a producer.

    b. At this point your Producer account has been created and you are registered as a producer. Feel free to cast your votes now and watch the testnet monitor reflect your actions.



## Supported Operating Systems
Telos currently supports the following operating systems:  
1. Amazon 2017.09 and higher
2. Centos 7
3. Fedora 25 and higher (Fedora 27 recommended)
4. Mint 18
5. Ubuntu 16.04 (Ubuntu 16.10 recommended)
6. Ubuntu 18.04
7. MacOS Darwin 10.12 and higher (MacOS 10.13.x recommended)

# Resources
1. [Telos Website](https://telosfoundation.io)
2. [Telos Blog](https://medium.com/@teloslogical)
3. [Telos Twitter](https://twitter.com/HelloTelos)
4. [Telos Documentation Wiki](https://github.com/Telos-Foundation/telos/wiki)
5. [Telos Community Telegram Group](https://t.me/TheTelosFoundation)

Telos is released under the open source MIT license and is offered “AS IS” without warranty of any kind, express or implied. Any security provided by the Telos software depends in part on how it is used, configured, and deployed. Telos is built upon many third-party libraries such as Binaryen (Apache License) and WAVM (BSD 3-clause) which are also provided “AS IS” without warranty of any kind. Without limiting the generality of the foregoing, Telos Foundation makes no representation or guarantee that Telos or any third-party libraries will perform as intended or will be free of errors, bugs or faulty code. Both may fail in large or small ways that could completely or partially limit functionality or compromise computer systems. If you use or implement Telos, you do so at your own risk. In no event will Telos Foundation be liable to any party for any damages whatsoever, even if it had been advised of the possibility of damage.