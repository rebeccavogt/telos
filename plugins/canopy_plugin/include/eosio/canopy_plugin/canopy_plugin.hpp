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
using std::shared_ptr;

typedef shared_ptr<class canopy_plugin_impl> canopy_ptr;
typedef shared_ptr<const class canopy_plugin_impl> canopy_const_ptr;

namespace canopy_apis {
    class read_only{
        canopy_const_ptr canopy;
    public:

        read_only(canopy_const_ptr&& canopy)
                : canopy(canopy) {}


        struct get_provider_params {
            chain::name provider;
        };

        struct get_provider_results {
            chain::name account;
            uint8_t status;
            string ipfs_endpoint;
            asset disk_balance;
        };

        struct get_hello_world_params{
            string hello;
        };

        struct get_hello_world_results{
            string world;
        };

        get_hello_world_results get_hello_world( const get_hello_world_params& )const;

    };

}

class canopy_plugin : public appbase::plugin<canopy_plugin> {

    public:

    APPBASE_PLUGIN_REQUIRES((http_plugin)(chain_plugin))

    canopy_plugin();
    virtual ~canopy_plugin();
    
    virtual void set_program_options(options_description&, options_description& cfg) override;
    
    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();


    //get_provider_results get_provider(get_provider_params p);

    canopy_apis::read_only  get_read_only_api()const { return canopy_apis::read_only(canopy_const_ptr(my)); }

    private:
    canopy_ptr my;
};



} //namespace eosio

FC_REFLECT(eosio::canopy_apis::read_only::get_hello_world_params, (hello))
FC_REFLECT(eosio::canopy_apis::read_only::get_hello_world_results, (world))

//FC_REFLECT(eosio::canopy_plugin::get_provider_params, (provider))
//FC_REFLECT(eosio::canopy_plugin::rmvfile_request_params, (ipfs_cid)(payer))
