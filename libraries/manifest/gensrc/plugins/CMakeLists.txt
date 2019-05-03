
add_library( sophiatx_plugins
   mf_plugins.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/sophiatx/manifest/plugins.hpp
)

target_link_libraries( sophiatx_plugins appbase fc

   witness_plugin

   account_history_api_plugin

   subscribe_api_plugin

   custom_api_plugin

   witness_api_plugin

   network_broadcast_api_plugin

   database_api_plugin

   alexandria_api_plugin

   chain_api_plugin

   account_by_key_api_plugin

   block_api_plugin

   debug_node_api_plugin

   multiparty_messaging_plugin

   account_history_plugin

   chain_plugin

   debug_node_plugin

   webserver_plugin

   template_plugin

   p2p_plugin

   block_log_info_plugin

   account_by_key_plugin

)

target_include_directories( sophiatx_plugins
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
   )

INSTALL( TARGETS
   sophiatx_plugins

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)