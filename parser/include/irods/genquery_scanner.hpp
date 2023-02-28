#ifndef IRODS_GENQUERY_SCANNER_HPP
#define IRODS_GENQUERY_SCANNER_HPP

#ifndef yyFlexLexerOnce
#  undef yyFlexLexer
#  define yyFlexLexer Genquery_FlexLexer // the trick with prefix; no namespace here :(
#  include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL irods::experimental::api::genquery::parser::symbol_type irods::experimental::api::genquery::scanner::get_next_token()

#include "parser.hpp"

namespace irods::experimental::api::genquery
{
    class wrapper;

    class scanner : public yyFlexLexer
    {
    public:
        explicit scanner(wrapper& wrapper)
            : wrapper_(wrapper)
        {
        }

        virtual parser::symbol_type get_next_token();

        //void LexerError(const char* _msg) override;

    private:
        wrapper& wrapper_;
    };
} // namespace irods::experimental::api::genquery

#endif // IRODS_GENQUERY_SCANNER_HPP
