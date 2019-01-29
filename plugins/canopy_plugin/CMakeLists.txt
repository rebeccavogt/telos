file(GLOB HEADERS "include/eosio/canopy_plugin/*.hpp")
add_library( canopy_plugin
             canopy_plugin.cpp
             ${HEADERS} )

target_link_libraries( canopy_plugin appbase fc )
target_include_directories( canopy_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
