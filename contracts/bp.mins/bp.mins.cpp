#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosio.system/eosio.system.hpp>

using namespace eosio;
using namespace std;

class bpmins : public eosio::contract {
    public:
        bpmins(account_name self) : eosio::contract(self), producermins(_self, _self) {}

    void setmins(account_name owner) {
        require_auth(owner);

        typedef eosio::multi_index<N(producers), eosiosystem::producer_info> producer_info_t;
        producer_info_t _producers(N(eosio), N(eosio));
        auto prod = _producers.get(owner,    "user is not a producer");
        eosio_assert(prod.is_active == true, "user is not an active producer");
        eosio_assert(prod.total_votes > 0.0, "user has no votes");

        auto itr = producermins.find(owner);
        if (itr != producermins.end()) {
            producermins.modify(itr, owner, [&](auto& f) {
                f.owner = owner;
                f.last_update = now();
            });
        } else {
            producermins.emplace(owner, [&](auto& f) {
                f.owner = owner;
                f.last_update = now();
            });
        }
    }

    void delmins(account_name prod) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        producermins.erase(itr);
    }

    void setorg(account_name prod, string org_name, string website, string coc, string od, string email) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");

        producermins.modify(itr, 0, [&](auto& f) {
            f.last_update = now();
            f.org_name = org_name;
            f.website = website;
            f.code_of_conduct = coc;
            f.ownership_disclosure = od;
            f.email = email;
        });
    }

    void setlocation(account_name prod, string location, string country, string latitude, string longitude) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");

        producermins.modify(itr, 0, [&](auto& f) {
            f.last_update = now();
            f.location = location;
            f.country = country;
            f.latitude = latitude;
            f.longitude = longitude;
        });
    }

    void setsocial(account_name prod, string steemit, string twitter, string youtube, string facebook, string github, string telegram, string wechat) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");

        producermins.modify(itr, 0, [&](auto& f) {
            f.last_update = now();
            f.steemit = steemit;
            f.twitter = twitter;
            f.youtube = youtube;
            f.facebook = facebook;
            f.github = github;
            f.telegram = telegram;
            f.wechat = wechat;
        });
    }

    void addnode(account_name prod, string node_name, string node_type, string location, string country, string latitude, string longitude, string p2p, string bnet, string api, string ssl) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");
        producermin p = *itr;

        node_info ni {
            node_name, 
            node_type, 
            location, 
            country, 
            latitude, 
            longitude,
            p2p,
            bnet,
            api,
            ssl
        };

        auto temp_nodes = p.nodes;
        temp_nodes.emplace_back(ni);

        producermins.modify(itr, 0, [&](auto& f) {
            f.last_update = now();
            f.nodes = temp_nodes;
        });
    }

    void delnode(account_name prod, string node_name) {
        require_auth(prod);
        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");
        producermin p = *itr;

        auto node_itr = p.nodes.begin();

        while (node_itr != p.nodes.end()) {
            if (node_itr->node_name == node_name) {
                p.nodes.erase(node_itr);
                break;
            }
            node_itr++;
        }

        producermins.modify(itr, 0, [&](auto& f) {
            f.last_update = now();
            f.nodes = p.nodes;
        });

    }

    void validatebp(account_name validator, account_name prod) {
        require_auth(validator);

        auto itr = producermins.find(prod);
        eosio_assert(itr != producermins.end(), "producer not found in producermins table");
        producermin p = *itr;

        //NOTE: excluding validatebp() from triggering last_update
        producermins.modify(itr, 0, [&](auto& f) {
            f.last_validate = now();
            f.last_validator = validator;
        });
    }


    private:
        
        struct node_info {
            string node_name;
            string node_type;

            //location info //NOTE: maybe have lat/long as doubles? 
            string location; //city, state
            string country;
            string latitude;
            string longitude;

            //connection info
            string p2p_endpoint;
            string bnet_endpoint;
            string api_endpoint;
            string ssl_endpoint;
        };

        // @abi table producermins i64
        struct producermin {
            account_name owner;
            uint32_t last_update;
            uint32_t last_validate;
            account_name last_validator;

            //org info
            string org_name;
            string website;
            string code_of_conduct;
            string ownership_disclosure;
            string email;

            //location info
            string location; //city, state
            string country;
            string latitude;
            string longitude;

            //social handles
            string steemit;
            string twitter;
            string youtube;
            string facebook;
            string github;
            string telegram;
            string wechat;

            //list of nodes
            vector<node_info> nodes;

            uint64_t primary_key() const { return owner; }
            EOSLIB_SERIALIZE(producermin, (owner)(last_update)(last_validate)(last_validator)
                (org_name)(website)(code_of_conduct)(ownership_disclosure)(email)
                (location)(country)(latitude)(longitude)
                (steemit)(twitter)(youtube)(facebook)(github)(telegram)(wechat)
                (nodes))
        };

        typedef eosio::multi_index<N(producermins), producermin> producermins_table;
        producermins_table producermins;
};

EOSIO_ABI(bpmins, (setmins)(delmins)(setorg)(setlocation)(setsocial)(addnode)(delnode)(validatebp))