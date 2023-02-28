#include "irods/genquery_wrapper.hpp"

#include <sstream>

namespace irods::experimental::api::genquery
{
    wrapper::wrapper(std::istream* _is_ptr)
        : scanner_{*this}
        , parser_{scanner_, *this}
        , select_{}
        , location_{}
    {
        scanner_.switch_streams(_is_ptr, nullptr);
        parser_.parse(); // TODO: handle error here
    } // wrapper

    auto wrapper::parse(std::istream& _is) -> select
    {
        wrapper wrapper(&_is);
        return wrapper.select_;
    } // parse

    auto wrapper::parse(const char* _s) -> select
    {
        std::istringstream iss(_s);
        return parse(iss);
    } // parse

    auto wrapper::parse(const std::string& _s) -> select
    {
        std::istringstream iss(_s);
        return parse(iss);
    } // parse

    auto wrapper::increment_location(std::uint64_t _location) -> void
    {
        location_ += _location;
    } // increment_location

    auto wrapper::location() const -> std::uint64_t
    {
        return location_;
    } // location
} // namespace irods::experimental::api::genquery
