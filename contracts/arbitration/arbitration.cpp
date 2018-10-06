/**
 * Arbitration Implementation. See function bodies for further notes.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#include "arbitration.hpp"

arbitration::arbitration(account_name self) : contract(self) {

}

arbitration::~arbitration() {

}

void arbitration::setconfig(uint16_t max_arbs, uint32_t default_time) {
    require_auth(_self);

    //TODO: expand as struct is developed
    config config_struct = config{
        _self, //publisher
        max_arbs, //max_concurrent_arbs
        default_time //default_time_limit
    };

    config_singleton _config(_self, _self);
    _config.set(config_struct, _self);

    print("\nSettings Configured");
}

