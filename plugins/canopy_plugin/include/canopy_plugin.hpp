/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class canopy_plugin : public appbase::plugin<template_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    canopy_plugin();
    virtual ~canopy_plugin();
    
    APPBASE_PLUGIN_REQUIRES()
    virtual void set_program_options(options_description&, options_description& cfg) override;
    
    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::unique_ptr<class template_plugin_impl> my;
};

}
