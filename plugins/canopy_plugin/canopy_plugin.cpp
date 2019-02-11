/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#include <eosio/canopy_plugin/canopy_plugin.hpp>

namespace eosio {
    
    static appbase::abstract_plugin& _canopy_plugin = app().register_plugin<canopy_plugin>();

class canopy_plugin_impl {
    public:

    canopy_plugin_impl(){}
    ~canopy_plugin_impl(){}

    int64_t billing_rate_per_chunk = 1;
    string ipfs_node = "http://";
    string ipfs_peer_id = "Qm...";
    name provider = name("testprovider");
    name pool_name = name("testpool");

    // read_write db = get_read_write_api();
    // abi_def canopy_abi = get_abi(&db, name("teloscanopy"));
    // auto table_type = get_table_type( abi, "accounts" );

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

void canopy_plugin::plugin_startup() {
    
    ilog("starting canopy_plugin...");

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

canopy_plugin::get_provider_results canopy_plugin::get_provider(get_provider_params p) {
    //get_provider_results results;

    eosio::chain_apis::read_only::get_table_rows_params table_params;
    table_params.code = chain::name("teloscanopy");
    table_params.scope = "teloscanopy";
    table_params.table = chain::name("users");
    table_params.table_key = "account";

    chain_apis::read_only::get_table_rows_result result = chain_apis::read_only::get_table_rows(table_params);
    return results;
}

} //namespace eosio
