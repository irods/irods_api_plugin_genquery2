#include "irods/plugins/api/private/genquery2_common.hpp"
#include "irods/plugins/api/genquery2_common.h" // For API plugin number.

#include <irods/apiHandler.hpp>
#include <irods/irods_rs_comm_query.hpp>
#include <irods/irods_server_properties.hpp>
#include <irods/procApiRequest.h>
#include <irods/rodsConnect.h>
#include <irods/rodsDef.h>
#include <irods/rodsErrorTable.h>

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
#  include <irods/irods_configuration_keywords.hpp>
#  include <irods/irods_get_full_path_for_config_file.hpp>
#  include <irods/irods_server_properties.hpp>
#  include <irods/rodsLog.h>
#  include <json.hpp>
#else
#  include <irods/catalog.hpp> // Requires linking against libnanodbc.so
#  include <irods/irods_logger.hpp>
#  include <nlohmann/json.hpp>
#endif // IRODS_ENABLE_42X_COMPATIBILITY

#include "irods/genquery2_driver.hpp"
#include "irods/genquery2_sql.hpp"

#include <fmt/format.h>
#include <nanodbc/nanodbc.h>

#include <cstring> // For strdup.

namespace
{
#ifndef IRODS_ENABLE_42X_COMPATIBILITY
	using log_api = irods::experimental::log::api;
#endif // IRODS_ENABLE_42X_COMPATIBILITY

	//
	// Function Prototypes
	//

	auto call_genquery2(irods::api_entry*, RsComm*, const genquery2_input*, char**) -> int;

	auto rs_genquery2(RsComm*, const genquery2_input*, char**) -> int;

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
	// catalog.hpp and catalog_utilities.hpp are not included in the dev package, so copy
	// the implementation of this function to ease compatibility between 4.2 and later versions
	// of iRODS.
	auto new_database_connection(bool _read_server_config = false) -> std::tuple<std::string, nanodbc::connection>;
#endif // IRODS_ENABLE_42X_COMPATIBILITY

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
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, "Invalid input: received nullptr for message pointer and/or response pointer.");
#else
			log_api::error("Invalid input: received nullptr for message pointer and/or response pointer.");
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return SYS_INVALID_INPUT_PARAM;
		}

		// Redirect to the catalog service provider based on the user-provided zone.
		// This allows clients to query federated zones.
		//
		// If the client did not provide a zone, getAndConnRcatHost() will operate as if the
		// client provided the local zone's name.
		if (_input->zone) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG8,
			        "GenQuery2 API endpoint received: query_string=[%s], zone=[%s]",
			        _input->query_string,
			        _input->zone);
#else
			log_api::trace(
				"GenQuery2 API endpoint received: query_string=[{}], zone=[{}]", _input->query_string, _input->zone);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
		}
		else {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(
				LOG_DEBUG8, "GenQuery2 API endpoint received: query_string=[%s], zone=[nullptr]", _input->query_string);
#else
			log_api::trace("GenQuery2 API endpoint received: query_string=[{}], zone=[nullptr]", _input->query_string);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
		}

		rodsServerHost* host_info{};

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		if (const auto ec = getAndConnRcatHost(_comm, MASTER_RCAT, _input->zone, &host_info); ec < 0) {
			rodsLog(LOG_ERROR, "Could not connect to remote zone [%s].", _input->zone);
			return ec;
		}
#else
		if (const auto ec = getAndConnRcatHost(_comm, PRIMARY_RCAT, _input->zone, &host_info); ec < 0) {
			log_api::error("Could not connect to remote zone [{}].", _input->zone);
			return ec;
		}
#endif // IRODS_ENABLE_42X_COMPATIBILITY

		// Return an error if the host information does not point to the zone of interest.
		// getAndConnRcatHost() returns the local zone if the target zone does not exist. We must catch
		// this situation to avoid querying the wrong catalog.
		if (host_info && _input->zone) {
			const std::string_view resolved_zone = static_cast<zoneInfo*>(host_info->zoneInfo)->zoneName;

			if (resolved_zone != _input->zone) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, "Could not find zone [%s].", _input->zone);
#else
				log_api::error("Could not find zone [{}].", _input->zone);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return SYS_INVALID_ZONE_NAME;
			}
		}

		if (host_info->localFlag != LOCAL_HOST) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG8, "Redirecting request to remote zone [%s].", _input->zone);
#else
			log_api::trace("Redirecting request to remote zone [{}].", _input->zone);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

			return procApiRequest(
				host_info->conn,
				IRODS_APN_GENQUERY2,
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				const_cast<genquery2_input*>(_input),
#else
				_input,
#endif // IRODS_ENABLE_42X_COMPATIBILITY
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
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				// clang-format off
				using map_type      = std::unordered_map<std::string, boost::any>;
				using key_path_type = irods::configuration_parser::key_path_t;
				// clang-format on

				const auto& db_plugin_config = irods::get_server_property<const map_type&>(
					key_path_type{irods::CFG_PLUGIN_CONFIGURATION_KW, irods::PLUGIN_TYPE_DATABASE});
				const auto& [key, value] = *std::begin(db_plugin_config);

				opts.database = key;
#else
#  ifdef IRODS_ENABLE_430_COMPATIBILITY
				const auto& config = irods::server_properties::instance().map();
#  else
				const auto handle = irods::server_properties::instance().map();
				const auto& config = handle.get_json();
#  endif // IRODS_ENABLE_430_COMPATIBILITY
				const auto& db = config.at(json::json_pointer{"/plugin_configuration/database"});
				opts.database = std::begin(db).key();
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			}

			opts.username = _comm->clientUser.userName; // TODO Handle remote users?
			opts.admin_mode = irods::is_privileged_client(*_comm);
			//opts.default_number_of_rows = 8; // TODO Can be pulled from the catalog on server startup.

			irods::experimental::genquery2::driver driver;

			if (const auto ec = driver.parse(_input->query_string); ec != 0) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, "Failed to parse GenQuery2 string. [error code=[%d]]", ec);
#else
				log_api::error("Failed to parse GenQuery2 string. [error code=[{}]]", ec);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return SYS_LIBRARY_ERROR;
			}

			const auto [sql, values] = gq::to_sql(driver.select, opts);

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG8, "Returning to client: [%s]", sql.c_str());
#else
			log_api::trace("Returning to client: [{}]", sql);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

			if (1 == _input->sql_only) {
				*_output = strdup(sql.c_str());
				return 0;
			}

			if (sql.empty()) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, "Could not generate SQL from GenQuery.");
#else
				log_api::error("Could not generate SQL from GenQuery.");
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return SYS_INVALID_INPUT_PARAM;
			}

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			auto [db_inst, db_conn] = new_database_connection();
#else
			auto [db_inst, db_conn] = irods::experimental::catalog::new_database_connection();
#endif // IRODS_ENABLE_42X_COMPATIBILITY

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
				for (std::remove_const_t<decltype(n_cols)> i = 0; i < n_cols; ++i) {
					json_row.push_back(row.get<std::string>(i, ""));
				}

				json_array.push_back(json_row);
				json_row.clear();
			}

			*_output = strdup(json_array.dump().c_str());
		}
		catch (const nanodbc::database_error& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, "Caught database exception while executing query: %s", e.what());
#else
			log_api::error("Caught database exception while executing query: {}", e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return SYS_LIBRARY_ERROR;
		}
		catch (const std::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, "Caught exception while executing query: %s", e.what());
#else
			log_api::error("Caught exception while executing query: {}", e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return SYS_LIBRARY_ERROR;
		}

		return 0;
	} // rs_genquery2

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
	auto new_database_connection(bool _read_server_config) -> std::tuple<std::string, nanodbc::connection>
	{
		const std::string dsn = [] {
			if (const char* dsn = std::getenv("irodsOdbcDSN"); dsn) {
				return dsn;
			}

			return "iRODS Catalog";
		}();

		std::string db_username;
		std::string db_password;
		std::string db_instance_name;

		if (_read_server_config) {
			std::string config_path;

			if (const auto error = irods::get_full_path_for_config_file("server_config.json", config_path); !error.ok())
			{
				rodsLog(LOG_ERROR, "Server configuration not found");
				throw std::runtime_error{"Failed to connect to catalog"};
			}

			rodsLog(LOG_DEBUG9, "Reading server configuration ...");

			nlohmann::json config;

			{
				std::ifstream config_file{config_path};
				config_file >> config;
			}

			const auto& db_plugin_config =
				config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_DATABASE);
			const auto& db_instance = db_plugin_config.front();

			db_instance_name = std::begin(db_plugin_config).key();
			db_username = db_instance.at(irods::CFG_DB_USERNAME_KW).get<std::string>();
			db_password = db_instance.at(irods::CFG_DB_PASSWORD_KW).get<std::string>();
		}
		else {
			// clang-format off
			using map_type      = std::unordered_map<std::string, boost::any>;
			using key_path_type = irods::configuration_parser::key_path_t;
			// clang-format on

			const auto& db_plugin_config = irods::get_server_property<const map_type&>(
				key_path_type{irods::CFG_PLUGIN_CONFIGURATION_KW, irods::PLUGIN_TYPE_DATABASE});
			const auto& [key, value] = *std::begin(db_plugin_config);
			const auto& db_instance = boost::any_cast<const map_type&>(value);

			db_instance_name = key;
			db_username = boost::any_cast<const std::string&>(db_instance.at(irods::CFG_DB_USERNAME_KW));
			db_password = boost::any_cast<const std::string&>(db_instance.at(irods::CFG_DB_PASSWORD_KW));
		}

		try {
			if (db_instance_name.empty()) {
				throw std::runtime_error{"Database instance name cannot be empty"};
			}

			rodsLog(LOG_DEBUG9, fmt::format("attempting connection to database using dsn [{}]", dsn).c_str());

			nanodbc::connection db_conn{dsn, db_username, db_password};

			if (db_instance_name == "mysql") {
				// MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT mode) to be
				// able to understand Postgres SQL. STRICT_TRANS_TABLES must be set too, otherwise
				// inserting NULL into a "NOT NULL" column does not produce an error.
				nanodbc::just_execute(db_conn, "set SESSION sql_mode = 'ANSI,STRICT_TRANS_TABLES'");
				nanodbc::just_execute(db_conn, "set character_set_client = utf8");
				nanodbc::just_execute(db_conn, "set character_set_results = utf8");
				nanodbc::just_execute(db_conn, "set character_set_connection = utf8");
			}

			return {db_instance_name, db_conn};
		}
		catch (const std::exception& e) {
			rodsLog(LOG_ERROR, e.what());
			throw std::runtime_error{"Failed to connect to catalog"};
		}
	} // new_database_connection
#endif // IRODS_ENABLE_42X_COMPATIBILITY
} //namespace

const operation_type op = rs_genquery2;
auto fn_ptr = reinterpret_cast<funcPtr>(call_genquery2);
