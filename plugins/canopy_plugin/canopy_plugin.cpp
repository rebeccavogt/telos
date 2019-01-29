/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
//#include <eosio/canopy_plugin/canopy_plugin.hpp>
#include <include/canopy_plugin.hpp>

namespace eosio {
    static appbase::abstract_plugin& _canopy_plugin = app().register_plugin<canopy_plugin>();

class canopy_plugin_impl {
    public:
};

canopy_plugin::canopy_plugin():my(new canopy_plugin_impl()){}
canopy_plugin::~canopy_plugin(){}

void canopy_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()("option-name", bpo::value<string>()->default_value("default value"), "Option Description");
}

void canopy_plugin::plugin_initialize(const variables_map& options) {
    try {
        if( options.count( "option-name" )) {
            // Handle the option
        }
    } 
    FC_LOG_AND_RETHROW()
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
    
    ilog("starting canopy_plugin");
    app().get_plugin<http_plugin>().add_api({
       CALL(addfile_request, 200),
       CALL(rmvfile_request, 200)
    });

    //TODO: grab provider pk, provider name, and ipfs endpoint(s) from config file

    //TODO: bind to ipfs node

    //TODO: load provider info from contract

    //TODO: request peer id from ipfs node, save to provider info

}

void canopy_plugin::plugin_shutdown() {
    // OK, that's enough magic

    //TODO: release info
}

} //namespace eosio
