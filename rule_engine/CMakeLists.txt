set(IRODS_RULE_ENGINE_PLUGIN_NAME irods_rule_engine_plugin-genquery2)

add_library(
  ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  MODULE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_compile_options(
  ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  PRIVATE
  -Wall
  -Wextra
  #-Werror
  -Wconversion
  -Wshadow
  -Wpedantic)

target_compile_definitions(
  ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS}
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG)

target_include_directories(
  ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/api_plugin/include>
  ${IRODS_INCLUDE_DIRS}
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)

target_link_libraries(
  ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  PRIVATE
  irods_server
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  dl)

if (IRODS_ENABLE_42X_COMPATIBILITY)
  target_compile_definitions(
    ${IRODS_RULE_ENGINE_PLUGIN_NAME}
    PRIVATE
    IRODS_ENABLE_42X_COMPATIBILITY)

  target_include_directories(
    ${IRODS_RULE_ENGINE_PLUGIN_NAME}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_JSON}/include)

  target_link_libraries(
    ${IRODS_RULE_ENGINE_PLUGIN_NAME}
    PRIVATE
	irods_common) # For rodsLog, etc.
else()
  target_link_libraries(
    ${IRODS_RULE_ENGINE_PLUGIN_NAME}
    PRIVATE
    nlohmann_json::nlohmann_json)
endif()

install(
  TARGETS ${IRODS_RULE_ENGINE_PLUGIN_NAME}
  LIBRARY DESTINATION ${IRODS_PLUGINS_DIRECTORY}/rule_engines)

#install(FILES ${CMAKE_SOURCE_DIR}/packaging/test_project_template_cpp_rule_engine_plugin.py
#        DESTINATION ${IRODS_HOME_DIRECTORY}/scripts/irods/test
#        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

#install(FILES ${CMAKE_SOURCE_DIR}/packaging/run_project_template_cpp_rule_engine_plugin_test.py
#        DESTINATION ${IRODS_HOME_DIRECTORY}/scripts
#        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
