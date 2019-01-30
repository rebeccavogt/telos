/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

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

    struct addfile_params {
        uint64_t ipfs_cid;
        chain::name payer;
    };
    
    //holds info needed for routing file to ipfs node
    struct addfile_results {
        uint32_t request_time;
        //uint16_t ipfs_chunks;
        //asset escrowed_bill;
    };

    struct acceptfile_params {
        uint64_t ipfs_cid;
        chain::name validator;
    };

    struct acceptfile_results {
        uint32_t accept_time;
    };

    // struct billing_info {
    //     uint16_t ipfs_chunks;
    //     asset final_bill; //NOTE: denominated by HDD token (represents 256KiB chunks)
    // };

    // struct rmvfile_params {
    //     chain::uint64_t ipfs_cid;
    //     chain::name payer;
    // };

    addfile_results addfile_request(const addfile_params&);
    acceptfile_results acceptfile_request(const acceptfile_params&);

    asset calculate_bill(uint32_t file_size_bytes);

private:
    std::unique_ptr<class canopy_plugin_impl> my;
};

} //namespace eosio

FC_REFLECT(eosio::canopy_plugin::addfile_params, (ipfs_cid)(payer))
//FC_REFLECT(eosio::canopy_plugin::rmvfile_request_params, (ipfs_cid)(payer))
