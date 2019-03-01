/**
 *
 *  @author Rebecca Vogt
 *  @copyright defined in telos/LICENSE.txt
 */
#include <eosio/canopy_api_plugin/canopy_api_plugin.hpp>

namespace eosio {
    static appbase::abstract_plugin& _canopy_api_plugin = app().register_plugin<canopy_api_plugin>();

    class canopy_api_plugin_impl {
    public:
        canopy_api_plugin_impl(){}
        ~canopy_api_plugin_impl(){}
    };

    canopy_api_plugin::canopy_api_plugin():my(new canopy_api_plugin_impl()){}
    canopy_api_plugin::~canopy_api_plugin(){}

    void canopy_api_plugin::set_program_options(options_description&, options_description& cfg) {
        cfg.add_options()
                ("option-name", bpo::value<string>()->default_value("default value"), "Option Description")
              ;
    }

    void canopy_api_plugin::plugin_initialize(const variables_map& options) {
        ilog("initializing canopy_api_plugin...");
        try {
            if( options.count( "option-name" )) {
                // Handle the option
            }
        }
        FC_LOG_AND_RETHROW()

        ilog("intialization complete");
    }

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CANOPY_RO_CALL(call_name) CALL(canopy, ro_api, canopy_apis::read_only, call_name)
//#define CANOPY_RW_CALL(call_name) CALL(canopy, rw_api, canopy_apis::read_write, call_name)

    void canopy_api_plugin::plugin_startup() {
        ilog("starting canopy_api_plugin...");
        auto ro_api = app().get_plugin<canopy_plugin>().get_read_only_api();
        //auto rw_api = app().get_plugin<canopy_plugin>().get_read_write_api();

        app().get_plugin<http_plugin>().add_api({
           CANOPY_RO_CALL(get_hello_world)
        });
    }

    void canopy_api_plugin::plugin_shutdown() {
        // OK, that's enough magic
        ilog("shutting down canopy_api_plugin...");

        ilog("shutdown complete");
    }

}

