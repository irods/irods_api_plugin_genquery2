#include "irods/plugins/api/private/genquery2_common.hpp"
#include "irods/plugins/api/genquery2_common.h" // For API plugin number.

#include <irods/apiHandler.hpp>
#include <irods/client_api_allowlist.hpp>
#include <irods/irods_version.h>
#include <irods/rcMisc.h>
#include <irods/rodsPackInstruct.h>

// The plugin factory function must always be defined.
extern "C" auto plugin_factory(
	[[maybe_unused]] const std::string& _instance_name, // NOLINT(bugprone-easily-swappable-parameters)
	[[maybe_unused]] const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
#  if IRODS_VERSION_INTEGER < 4003001
	irods::client_api_allowlist::instance().add(IRODS_APN_GENQUERY2);
#  else
	irods::client_api_allowlist::add(IRODS_APN_GENQUERY2);
#  endif
#endif // RODS_SERVER

	// clang-format off
	irods::apidef_t def{
		IRODS_APN_GENQUERY2,
		const_cast<char*>(RODS_API_VERSION),
		NO_USER_AUTH,
		NO_USER_AUTH,
		"GenQuery2_Input_PI",
		0,
		"STR_PI",
		0,
		op,
		"api_genquery2",
#if IRODS_VERSION_INTEGER < 4003001
		[](void* _p) {
			auto* q = static_cast<genquery2_input*>(_p);
			if (q->query_string) { std::free(q->query_string); }
			if (q->zone)         { std::free(q->zone); }
		},
#else
		[](void* _p) {
			auto* q = static_cast<genquery2_input*>(_p);
			if (q->query_string) { std::free(q->query_string); }
			if (q->zone)         { std::free(q->zone); }
		},
		irods::clearOutStruct_noop,
#endif
		fn_ptr
	};
	// clang-format on

	auto* api = new irods::api_entry{def}; // NOLINT(cppcoreguidelines-owning-memory)

	api->in_pack_key = "GenQuery2_Input_PI";
	api->in_pack_value = GenQuery2_Input_PI;

	api->out_pack_key = "STR_PI";
	api->out_pack_value = STR_PI;

	return api;
} // plugin_factory
