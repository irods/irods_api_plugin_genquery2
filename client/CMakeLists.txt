set(IRODS_EXECUTABLE_NAME iquery)

add_executable(
  ${IRODS_EXECUTABLE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_compile_definitions(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  #${IRODS_COMPILE_DEFINITIONS} # TODO Is this needed?
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE})

target_include_directories(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/api_plugin/include>
  ${IRODS_INCLUDE_DIRS}
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

target_link_libraries(
  ${IRODS_EXECUTABLE_NAME}
  PRIVATE
  irods_client
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)

if (IRODS_ENABLE_42X_COMPATIBILITY)
  target_compile_definitions(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
    IRODS_ENABLE_42X_COMPATIBILITY)

  target_include_directories(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_JSON}/include)

  target_link_libraries(
    ${IRODS_EXECUTABLE_NAME}
    PRIVATE
	irods_common) # For irods::exception.
endif()

add_dependencies(${IRODS_EXECUTABLE_NAME} irods_genquery2_client)

install(
  TARGETS ${IRODS_EXECUTABLE_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
