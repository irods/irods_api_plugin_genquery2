#include "irods/plugins/api/private/genquery2_common.hpp"
#include "irods/plugins/api/genquery2_common.h" // For API plugin number.

#include "irods/genquery2_driver.hpp"
#include "irods/genquery2_sql.hpp"

#include <irods/apiHandler.hpp>
#include <irods/catalog.hpp> // Requires linking against libnanodbc.so
#include <irods/irods_logger.hpp>
#include <irods/irods_rs_comm_query.hpp>
#include <irods/irods_server_properties.hpp>
#include <irods/irods_version.h>
#include <irods/procApiRequest.h>
#include <irods/rodsConnect.h>
#include <irods/rodsDef.h>
#include <irods/rodsErrorTable.h>

#include <fmt/format.h>
#include <nanodbc/nanodbc.h>
#include <nlohmann/json.hpp>

#include <cstring> // For strdup.

namespace
{
	using log_api = irods::experimental::log::api;

	//
	// Function Prototypes
	//

	auto call_genquery2(irods::api_entry*, RsComm*, const genquery2_input*, char**) -> int;

	auto rs_genquery2(RsComm*, const genquery2_input*, char**) -> int;

	//
	// Function Implementations
	//

	auto call_genquery2(irods::api_entry* _api, RsComm* _comm, const genquery2_input* _input, char** _output) -> int
	{
		return _api->call_handler<const genquery2_input*, char**>(_comm, _input, _output);
	} // call_genquery2

	auto rs_genquery2(RsComm* _comm, const genquery2_input* _input, char** _output) -> int
	{
		if (!_input || !_input->query_string || !_output) {
			log_api::error("Invalid input: received nullptr for message pointer and/or response pointer.");
			return SYS_INVALID_INPUT_PARAM;
		}

		// Redirect to the catalog service provider based on the user-provided zone.
		// This allows clients to query federated zones.
		//
		// If the client did not provide a zone, getAndConnRcatHost() will operate as if the
		// client provided the local zone's name.
		if (_input->zone) {
			log_api::trace(
				"GenQuery2 API endpoint received: query_string=[{}], zone=[{}]", _input->query_string, _input->zone);
		}
		else {
			log_api::trace("GenQuery2 API endpoint received: query_string=[{}], zone=[nullptr]", _input->query_string);
		}

		rodsServerHost* host_info{};

		if (const auto ec = getAndConnRcatHost(_comm, PRIMARY_RCAT, _input->zone, &host_info); ec < 0) {
			log_api::error("Could not connect to remote zone [{}].", _input->zone);
			return ec;
		}

		// Return an error if the host information does not point to the zone of interest.
		// getAndConnRcatHost() returns the local zone if the target zone does not exist. We must catch
		// this situation to avoid querying the wrong catalog.
		if (host_info && _input->zone) {
			const std::string_view resolved_zone = static_cast<zoneInfo*>(host_info->zoneInfo)->zoneName;

			if (resolved_zone != _input->zone) {
				log_api::error("Could not find zone [{}].", _input->zone);
				return SYS_INVALID_ZONE_NAME;
			}
		}

		if (host_info->localFlag != LOCAL_HOST) {
			log_api::trace("Redirecting request to remote zone [{}].", _input->zone);

			return procApiRequest(
				host_info->conn,
				IRODS_APN_GENQUERY2,
				_input,
				nullptr,
				reinterpret_cast<void**>(_output), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
				nullptr);
		}

		//
		// At this point, we assume we're connected to the catalog service provider.
		//

		try {
			using json = nlohmann::json;

			gq::options opts;

			{
				// Get the database type string from server_config.json.
#if IRODS_VERSION_INTEGER < 4003001
				const auto& config = irods::server_properties::instance().map();
#else
				const auto handle = irods::server_properties::instance().map();
				const auto& config = handle.get_json();
#endif
				const auto& db = config.at(json::json_pointer{"/plugin_configuration/database"});
				opts.database = std::begin(db).key();
			}

			opts.username = _comm->clientUser.userName; // TODO Handle remote users?
			opts.admin_mode = irods::is_privileged_client(*_comm);
			//opts.default_number_of_rows = 8; // TODO Can be pulled from the catalog on server startup.

			irods::experimental::genquery2::driver driver;

			if (const auto ec = driver.parse(_input->query_string); ec != 0) {
				log_api::error("Failed to parse GenQuery2 string. [error code=[{}]]", ec);
				return SYS_LIBRARY_ERROR;
			}

			const auto [sql, values] = gq::to_sql(driver.select, opts);

			log_api::trace("Returning to client: [{}]", sql);

			if (1 == _input->sql_only) {
				*_output = strdup(sql.c_str());
				return 0;
			}

			if (sql.empty()) {
				log_api::error("Could not generate SQL from GenQuery.");
				return SYS_INVALID_INPUT_PARAM;
			}

			auto [db_inst, db_conn] = irods::experimental::catalog::new_database_connection();

			nanodbc::statement stmt{db_conn};
			nanodbc::prepare(stmt, sql);

			for (std::vector<std::string>::size_type i = 0; i < values.size(); ++i) {
				stmt.bind(static_cast<short>(i), values.at(i).c_str());
			}

			auto json_array = json::array();
			auto json_row = json::array();

			auto row = nanodbc::execute(stmt);
			const auto n_cols = row.columns();

			while (row.next()) {
				for (std::remove_cvref_t<decltype(n_cols)> i = 0; i < n_cols; ++i) {
					json_row.push_back(row.get<std::string>(i, ""));
				}

				json_array.push_back(json_row);
				json_row.clear();
			}

			*_output = strdup(json_array.dump().c_str());
		}
		catch (const nanodbc::database_error& e) {
			log_api::error("Caught database exception while executing query: {}", e.what());
			return SYS_LIBRARY_ERROR;
		}
		catch (const std::exception& e) {
			log_api::error("Caught exception while executing query: {}", e.what());
			return SYS_LIBRARY_ERROR;
		}

		return 0;
	} // rs_genquery2
} //namespace

const operation_type op = rs_genquery2;
auto fn_ptr = reinterpret_cast<funcPtr>(call_genquery2);
