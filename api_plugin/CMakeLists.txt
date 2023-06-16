set(IRODS_MODULE_NAME_PREFIX irods_genquery2)

foreach (IRODS_MODULE_VARIANT IN ITEMS client server)
  set(IRODS_MODULE_NAME ${IRODS_MODULE_NAME_PREFIX}_${IRODS_MODULE_VARIANT})

  add_library(
    ${IRODS_MODULE_NAME}
    MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/${IRODS_MODULE_VARIANT}.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_factory.cpp)
  
#  set_target_properties(${IRODS_MODULE_NAME} PROPERTIES VERSION ${IRODS_VERSION})
#  set_target_properties(${IRODS_MODULE_NAME} PROPERTIES SOVERSION ${IRODS_VERSION})
  set_target_properties(${IRODS_MODULE_NAME} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
  set_target_properties(${IRODS_MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

  if (IRODS_ENABLE_430_COMPATIBILITY)
    target_compile_definitions(
      ${IRODS_MODULE_NAME}
      PRIVATE
      IRODS_ENABLE_430_COMPATIBILITY)
  elseif (IRODS_ENABLE_42X_COMPATIBILITY)
    target_compile_definitions(
      ${IRODS_MODULE_NAME}
      PRIVATE
      IRODS_ENABLE_42X_COMPATIBILITY)
  
    target_include_directories(
      ${IRODS_MODULE_NAME}
      PRIVATE
  	  ${IRODS_EXTERNALS_FULLPATH_JSON}/include)
  endif()

  target_compile_definitions(
    ${IRODS_MODULE_NAME}
    PRIVATE
    #${IRODS_COMPILE_DEFINITIONS} # TODO Is this needed?
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE})
  
  target_include_directories(
    ${IRODS_MODULE_NAME}
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/parser>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/parser/include>
    ${IRODS_INCLUDE_DIRS}
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    ${IRODS_EXTERNALS_FULLPATH_NANODBC}/include
    ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)
  
  target_link_libraries(
    ${IRODS_MODULE_NAME}
    PRIVATE
    irods_plugin_dependencies
    irods_common
    irods_${IRODS_MODULE_VARIANT}
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    ${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so)

  add_dependencies(${IRODS_MODULE_NAME} irods_genquery2_parser)

  install(
    TARGETS ${IRODS_MODULE_NAME}
    LIBRARY DESTINATION "${IRODS_PLUGINS_DIRECTORY}/api")
endforeach()

target_compile_definitions(
  ${IRODS_MODULE_NAME_PREFIX}_server
  PRIVATE
  RODS_SERVER
  ENABLE_RE
  IRODS_ENABLE_SYSLOG)

target_link_libraries(
  ${IRODS_MODULE_NAME_PREFIX}_server
  PRIVATE
  $<TARGET_OBJECTS:irods_genquery2_parser>)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/genquery2_common.h"
  #"${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/rc_genquery2.h"
  #"${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/rs_genquery2.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods/plugins/api")

#set(IRODS_PLUGIN_INVOCATION_FUNCTIONS irods_genquery2_invocation_functions)
#
#add_library(
#  ${IRODS_PLUGIN_INVOCATION_FUNCTIONS}
#  SHARED
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/rc_genquery2.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/rs_genquery2.cpp)
#
#target_include_directories(
#  ${IRODS_MODULE_NAME}
#  PRIVATE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#  ${IRODS_INCLUDE_DIRS}
#  #${IRODS_EXTERNALS_FULLPATH_BOOST}/include
#  #${IRODS_EXTERNALS_FULLPATH_FMT}/include
#  #${IRODS_EXTERNALS_FULLPATH_NANODBC}/include
#  #${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)
#  )
#
#target_link_libraries(
#  ${IRODS_PLUGIN_INVOCATION_FUNCTIONS}
#  PRIVATE
#  irods_server)
