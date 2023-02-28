#ifndef IRODS_GENQUERY_VERTEX_PROPERTY_HPP
#define IRODS_GENQUERY_VERTEX_PROPERTY_HPP

#include <string_view>

namespace irods::experimental::genquery
{
    struct vertex_property
    {
        std::string_view table_name;
    }; // struct vertex_property
} // namespace irods::experimental::genquery

#endif // IRODS_GENQUERY_VERTEX_PROPERTY_HPP
