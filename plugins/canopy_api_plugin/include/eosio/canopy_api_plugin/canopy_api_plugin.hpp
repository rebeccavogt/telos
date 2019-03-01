/**
 *
 *  @author Rebecca Vogt
 *  @copyright defined in telos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/canopy_plugin/canopy_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

namespace eosio {

    using namespace appbase;

    class canopy_api_plugin : public appbase::plugin<canopy_api_plugin> {
    public:

        APPBASE_PLUGIN_REQUIRES((http_plugin)(chain_plugin))

        canopy_api_plugin();
        virtual ~canopy_api_plugin();


        virtual void set_program_options(options_description&, options_description& cfg) override;

        void plugin_initialize(const variables_map& options);
        void plugin_startup();
        void plugin_shutdown();


    private:
        std::unique_ptr<class canopy_api_plugin_impl> my;
    };

}

