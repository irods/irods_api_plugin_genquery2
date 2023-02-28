#ifndef IRODS_GENQUERY_EDGE_PROPERTY_HPP
#define IRODS_GENQUERY_EDGE_PROPERTY_HPP

#include <string_view>

namespace irods::experimental::genquery
{
    struct edge_property
    {
        std::string_view join_condition;
    }; // struct edge_property
} // namespace irods::experimental::genquery

#endif // IRODS_GENQUERY_EDGE_PROPERTY_HPP
