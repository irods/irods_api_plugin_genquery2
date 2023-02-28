#ifndef IRODS_GENQUERY_WRAPPER_HPP
#define IRODS_GENQUERY_WRAPPER_HPP

#include "irods/genquery_ast_types.hpp"
#include "irods/genquery_scanner.hpp"
#include "parser.hpp"

#include <cstdint>
#include <string>

namespace irods::experimental::api::genquery
{
    class wrapper
    {
    public:
        explicit wrapper(std::istream* _is_ptr);

        static auto parse(std::istream& _is) -> select;
        static auto parse(const char* _s) -> select;
        static auto parse(const std::string& _s) -> select;

        friend class parser;
        friend class scanner;

    private:
        auto increment_location(std::uint64_t _count) -> void;
        auto location() const -> std::uint64_t;

        scanner scanner_;
        parser parser_;
        select select_;
        std::uint64_t location_;
    };
} // namespace irods::experimental::api::genquery

#endif // IRODS_GENQUERY_WRAPPER_HPP
