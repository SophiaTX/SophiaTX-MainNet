
add_library( steem_plugins
   mf_plugins.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/steem/manifest/plugins.hpp
)

target_link_libraries( steem_plugins appbase fc

   smt_test_plugin

   witness_plugin

   account_history_api_plugin

   witness_api_plugin

   network_broadcast_api_plugin

   database_api_plugin

   chain_api_plugin

   account_by_key_api_plugin

   block_api_plugin

   debug_node_api_plugin

   condenser_api_plugin

   account_history_plugin

   chain_plugin

   debug_node_plugin

   webserver_plugin

   p2p_plugin

   block_log_info_plugin

   account_by_key_plugin

)

target_include_directories( steem_plugins
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
   )

INSTALL( TARGETS
   steem_plugins

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)