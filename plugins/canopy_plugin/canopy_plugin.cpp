/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
//#include <eosio/canopy_plugin/canopy_plugin.hpp>
#include <eosio/canopy_plugin/canopy_plugin.hpp>

namespace eosio {
    static appbase::abstract_plugin& _canopy_plugin = app().register_plugin<canopy_plugin>();

class canopy_plugin_impl {
    public:

    int64_t billing_rate_per_chunk = 1;
    string ipfs_node = "http://";
    string ipfs_peer_id = "Qm...";
    name provider = name("testprovider");
    name pool_name = name("testpool");

};

canopy_plugin::canopy_plugin():my(new canopy_plugin_impl()){}
canopy_plugin::~canopy_plugin(){}

void canopy_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()("option-name", bpo::value<string>()->default_value("default value"), "Option Description");
}

void canopy_plugin::plugin_initialize(const variables_map& options) {

    ilog("initializing canopy_plugin...");

    try {
        if( options.count( "option-name" )) {
            // Handle the option
        }
    } 
    FC_LOG_AND_RETHROW()

    ilog("intialization complete");
}

#define CALL(call_name, http_response_code)                                                                         \
{                                                                                                                   \
    std::string("/v1/canopy/" #call_name), [this](string, string body, url_response_callback cb) mutable {          \
        try {                                                                                                       \
            if (body.empty())                                                                                       \
                body = "{}";                                                                                        \
            auto result = call_name(fc::json::from_string(body).as<login_plugin::call_name##_params>());            \
            cb(http_response_code, fc::json::to_string(result));                                                    \
        } catch (...) {                                                                                             \
            http_plugin::handle_exception("canopy", #call_name, body, cb);                                          \
        }                                                                                                           \
    }                                                                                                               \
}

void canopy_plugin::plugin_startup() {
    
    ilog("starting canopy_plugin...");

    // app().get_plugin<http_plugin>().add_api({
    //    CALL(addfile_request, 200),
    //    CALL(rmvfile_request, 200)
    // });

    //TODO: grab provider pk and provider name from config file

    //TODO: load billing rates from contract

    //TODO: bind to ipfs node (loaded endpoints from contract)

    //TODO: load provider info from contract

    //TODO: request peer id from ipfs node, save to provider info

    ilog("startup complete");
}

void canopy_plugin::plugin_shutdown() {

    ilog("shutting down canopy_plugin...");

    //TODO: update provider status?

    //TODO: release info

    ilog("shutdown complete");
}

canopy_plugin::addfile_results canopy_plugin::addfile_request(const addfile_params&) {
   
    //TODO: check file doesn't exist on contract (in addrequests table?)

    // const auto &d = db.db(); //TODO: shame whoever named this
    // const account_object& d.get_account(my->provider);

    //TODO: check file isnt already in IPFS? If so, attempt to add. If already there, break.

    //TODO: forward file and add request to IPFS node

    return addfile_results{fc::time_point::now().sec_since_epoch()};
}

asset canopy_plugin::calculate_bill(uint32_t file_size_bytes) {
    return asset(int64_t(file_size_bytes / my->billing_rate_per_chunk + 1), symbol(0, "HDD"));
}

} //namespace eosio
