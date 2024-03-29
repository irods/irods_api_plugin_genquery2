set(IRODS_EXECUTABLE_NAME iquery)

add_executable(
  ${IRODS_EXECUTABLE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_compile_definitions(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS}
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE})

target_include_directories(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_IRODS_PLUGIN_SOURCE_DIR}/api_plugin/include>
  $<BUILD_INTERFACE:${CMAKE_IRODS_PLUGIN_SOURCE_DIR}/parser/include>
  ${IRODS_INCLUDE_DIRS}
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

target_link_libraries(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  irods_client
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so)

if (${IRODS_VERSION} VERSION_GREATER "4.3.1")
  target_link_libraries(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
    fmt::fmt)
else()
  target_include_directories(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

  target_link_libraries(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
endif()

add_dependencies(${IRODS_EXECUTABLE_NAME} irods_genquery2_client)

install(
  TARGETS ${IRODS_EXECUTABLE_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
