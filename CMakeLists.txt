cmake_minimum_required(VERSION 3.11.0 FATAL_ERROR)

option(IRODS_ENABLE_430_COMPATIBILITY "Enables compatibility with iRODS 4.3.0." YES) # Default to YES for community.
option(IRODS_ENABLE_42X_COMPATIBILITY "Enables compatibility with iRODS 4.2.X." NO)  # Default to NO for community.

if (IRODS_ENABLE_430_COMPATIBILITY AND IRODS_ENABLE_42X_COMPATIBILITY)
  message(FATAL_ERROR "IRODS_ENABLE_430_COMPATIBILITY and IRODS_ENABLE_42X_COMPATIBILITY options are incompatible.")
endif()

find_package(IRODS REQUIRED CONFIG)

set(IRODS_PLUGIN_REVISION "0")
set(IRODS_PLUGIN_VERSION "${IRODS_VERSION}.${IRODS_PLUGIN_REVISION}")

set(IRODS_PACKAGE_REVISION "1")

if (IRODS_ENABLE_42X_COMPATIBILITY)
  set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)
else()
  include(RequireOutOfSourceBuild)
  include(IrodsCXXCompiler)
endif()

set(CMAKE_CXX_STANDARD ${IRODS_CXX_STANDARD})
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE) # TODO Disable

# export-dynamic so stacktrace entries from executables have function names.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-rdynamic -Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-rdynamic -Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-rdynamic -Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")

project(
  irods_genquery2
  VERSION "${IRODS_VERSION}"
  LANGUAGES CXX)

set(CMAKE_SKIP_BUILD_RPATH OFF)
set(CMAKE_SKIP_INSTALL_RPATH OFF)
set(CMAKE_SKIP_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

include(${IRODS_TARGETS_PATH})
include(GNUInstallDirs)
include(UseLibCXX)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'. This is the correct setting for normal builds.")
endif()

# FIXME Why can't these be closer to the things that use them?
if (NOT IRODS_ENABLE_42X_COMPATIBILITY)
  find_package(nlohmann_json "3.6.1" REQUIRED)
endif()
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)
endif()

#add_compile_options(-Wall -Wextra -Werror -Wconversion -Wshadow -Wpedantic)
add_compile_options(-Wall -Wextra -Wconversion -Wshadow -Wpedantic)

add_subdirectory(parser)
add_subdirectory(api_plugin)
#add_subdirectory(microservice)
add_subdirectory(rule_engine)
add_subdirectory(client)

if (NOT IRODS_ENABLE_42X_COMPATIBILITY)
  include(IrodsCPackCommon)
  include(IrodsCPackPlatform)
endif()

set(IRODS_PACKAGE_NAME irods-experimental-api-plugin-genquery2)

list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts/irods")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts/irods/test")

set(CPACK_PACKAGE_VERSION ${IRODS_PLUGIN_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/postinst;")
#set(CPACK_DEBIAN_PACKAGE_BREAKS ${IRODS_PACKAGE_NAME})
#set(CPACK_DEBIAN_PACKAGE_REPLACES ${IRODS_PACKAGE_NAME})
set(CPACK_DEBIAN_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
if (IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
  set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}~${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}")
endif()
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
#set(CPACK_RPM_PACKAGE_OBSOLETES ${IRODS_PACKAGE_NAME})
#set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/postinst")
set(CPACK_RPM_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

set(CPACK_DEBIAN_PACKAGE_NAME ${IRODS_PACKAGE_NAME})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime (= ${IRODS_VERSION})")

set(CPACK_RPM_PACKAGE_NAME ${IRODS_PACKAGE_NAME})
set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}")

if (IRODS_ENABLE_42X_COMPATIBILITY)
  set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System") # Defined by IrodsCPackCommon.
  set(CPACK_PACKAGE_VENDOR "Renaissance Computing Institute <info@irods.org>")

  # Defined by IrodsCPackPlatform.
  if (NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR ${IRODS_CPACK_GENERATOR} CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  endif()
endif()

include(CPack)
