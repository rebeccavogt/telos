/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

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

    struct addfile_request_params {
        chain::uint64_t ipfs_cid;
        chain::name payer;
    };

    struct rmvfile_request_params {
        chain::uint64_t ipfs_cid;
        chain::name payer;
    };

private:
    std::unique_ptr<class canopy_plugin_impl> my;
};

} //namespace eosio

FC_REFLECT(eosio::canopy_plugin::addfile_request_params, (ipfs_cid)(payer))
FC_REFLECT(eosio::canopy_plugin::rmvfile_request_params, (ipfs_cid)(payer))
