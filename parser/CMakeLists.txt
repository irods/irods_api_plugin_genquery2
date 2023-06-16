find_package(FLEX 2.6.1 REQUIRED)
find_package(BISON 3.0.4 REQUIRED)

set(IRODS_FLEX_LEXER_NAME irods_lexer)
set(IRODS_BISON_PARSER_NAME irods_parser)

FLEX_TARGET(${IRODS_FLEX_LEXER_NAME} dsl/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(${IRODS_BISON_PARSER_NAME} dsl/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
ADD_FLEX_BISON_DEPENDENCY(${IRODS_FLEX_LEXER_NAME} ${IRODS_BISON_PARSER_NAME})

set(IRODS_PARSER_NAME irods_genquery2_parser)
set(IRODS_FLEX_OUTPUTS "FLEX_${IRODS_FLEX_LEXER_NAME}_OUTPUTS")
set(IRODS_BISON_OUTPUTS "BISON_${IRODS_BISON_PARSER_NAME}_OUTPUTS")

add_library(
  ${IRODS_PARSER_NAME}
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/src/genquery2_driver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/genquery2_sql.cpp
  ${${IRODS_FLEX_OUTPUTS}}
  ${${IRODS_BISON_OUTPUTS}})

set_target_properties(${IRODS_PARSER_NAME} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(${IRODS_PARSER_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if (IRODS_ENABLE_430_COMPATIBILITY)
  target_compile_definitions(
    ${IRODS_PARSER_NAME}
    PRIVATE
    IRODS_ENABLE_430_COMPATIBILITY)
elseif (IRODS_ENABLE_42X_COMPATIBILITY)
  target_compile_definitions(
    ${IRODS_PARSER_NAME}
    PRIVATE
    IRODS_ENABLE_42X_COMPATIBILITY)
endif()

target_compile_definitions(
  ${IRODS_PARSER_NAME}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG)

target_include_directories(
  ${IRODS_PARSER_NAME}
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  ${IRODS_EXTERNALS_FULLPATH_JSON}/include # TODO Should this be nlohmann_json::nlohmann_json?
  ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)

# TODO Perhaps this block isn't needed at all because we're generating an object file?
# The conditional allows the code to compile for 4.2.11 for some reason. 4.3 isn't affected by this. Why?
if (NOT IRODS_ENABLE_42X_COMPATIBILITY)
  target_link_libraries(
    ${IRODS_PARSER_NAME}
    PRIVATE
    #${FLEX_LIBRARIES} # This causes a compiler error when using C++ (i.e. undefined reference to yylex()).
    irods_server
    ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib/libc++.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
endif()
