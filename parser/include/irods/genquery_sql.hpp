#ifndef IRODS_GENQUERY_SQL_HPP
#define IRODS_GENQUERY_SQL_HPP

#include <string>
#include <string_view>

namespace irods::experimental::api::genquery
{
	struct select;

	struct options
	{
		std::string_view username;
		std::string_view database;
		std::uint16_t default_number_of_rows = 16;
		bool admin_mode = false;
	}; // struct options

	auto to_sql(const select& _select, const options& _opts) -> std::tuple<std::string, std::vector<std::string>>;
} // namespace irods::experimental::api::genquery

#endif // IRODS_GENQUERY_SQL_HPP
