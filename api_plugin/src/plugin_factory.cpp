#include "irods/plugins/api/private/genquery2_common.hpp"
#include "irods/plugins/api/genquery2_common.h" // For API plugin number.

#include <irods/apiHandler.hpp>
#include <irods/client_api_allowlist.hpp>
#include <irods/rcMisc.h>
#include <irods/rodsPackInstruct.h>

// The plugin factory function must always be defined.
extern "C" auto plugin_factory(
	[[maybe_unused]] const std::string& _instance_name, // NOLINT(bugprone-easily-swappable-parameters)
	[[maybe_unused]] const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
        // If your API endpoint is designed to be invocable by non-admins, then you need to
        // add the API plugin number to the allowlist.
#ifdef IRODS_ENABLE_430_COMPATIBILITY
	irods::client_api_allowlist::instance().add(IRODS_APN_GENQUERY2);
#else
	irods::client_api_allowlist::add(IRODS_APN_GENQUERY2);
#endif // IRODS_ENABLE_430_COMPATIBILITY
#endif // RODS_SERVER

	// TODO We need to be able to add API plugin numbers to the API plugin number map.

	// clang-format off
	irods::apidef_t def{
		IRODS_APN_GENQUERY2,
		const_cast<char*>(RODS_API_VERSION),
		NO_USER_AUTH,
		NO_USER_AUTH,
		"STR_PI",
		0,
		"STR_PI",
		0,
		op,
		"api_genquery2",
#ifdef IRODS_ENABLE_430_COMPATIBILITY
		irods::clearInStruct_noop,
#else
		irods::clearInStruct_noop,
		irods::clearOutStruct_noop, // TODO This blocks support for 4.2.
#endif // IRODS_ENABLE_430_COMPATIBILITY
		fn_ptr
	};
	// clang-format on

	auto* api = new irods::api_entry{def}; // NOLINT(cppcoreguidelines-owning-memory)

	// TODO Demonstrate how to add new serialization types.

	api->in_pack_key = "STR_PI";
	api->in_pack_value = STR_PI;

	api->out_pack_key = "STR_PI";
	api->out_pack_value = STR_PI;

	return api;
} // plugin_factory
