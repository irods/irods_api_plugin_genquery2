#ifndef IRODS_GENQUERY_SQL_HPP
#define IRODS_GENQUERY_SQL_HPP

//#include "irods/genquery_ast_types.hpp"

#include <string>
#include <string_view>

namespace irods::experimental::api::genquery
{
    struct select;

    struct options
    {
        std::string_view username;
        bool admin_mode = false;
    }; // struct options

    auto to_sql(const select& _select, const options& _opts) -> std::tuple<std::string, std::vector<std::string>>;
} // namespace irods::experimental::api::genquery

#endif // IRODS_GENQUERY_SQL_HPP
