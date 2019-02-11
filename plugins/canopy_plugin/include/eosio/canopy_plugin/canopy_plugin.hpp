/**
 *  
 *  @author Craig Branscom
 *  @copyright defined in telos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/json.hpp>

namespace eosio {

using namespace appbase;

class canopy_plugin : public appbase::plugin<canopy_plugin> {

    public:

    APPBASE_PLUGIN_REQUIRES((http_plugin)(chain_plugin))

    canopy_plugin();
    virtual ~canopy_plugin();
    
    virtual void set_program_options(options_description&, options_description& cfg) override;
    
    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    struct get_provider_params {
        chain::name provider;
    };

    struct get_provider_results {
        chain::name account;
        uint8_t status;
        string ipfs_endpoint;
        asset disk_balance;
    };

    get_provider_results get_provider(get_provider_params p);

    private:

    std::unique_ptr<class canopy_plugin_impl> my;
};

} //namespace eosio

FC_REFLECT(eosio::canopy_plugin::get_provider_params, (provider))
//FC_REFLECT(eosio::canopy_plugin::rmvfile_request_params, (ipfs_cid)(payer))
