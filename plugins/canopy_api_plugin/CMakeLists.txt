file(GLOB HEADERS "include/eosio/canopy_api_plugin/*.hpp")
add_library( canopy_api_plugin
             canopy_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( canopy_api_plugin canopy_plugin chain_plugin http_plugin appbase fc )
target_include_directories( canopy_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )


