file(GLOB HEADERS "include/eosio/tipfs_plugin/*.hpp")
add_library( tipfs_plugin
             tipfs_plugin.cpp
             ${HEADERS} )

target_link_libraries( tipfs_plugin appbase fc )
target_include_directories( tipfs_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
