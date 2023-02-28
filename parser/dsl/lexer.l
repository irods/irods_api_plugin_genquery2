%{
    #include "irods/genquery_scanner.hpp"
    #include "irods/genquery_wrapper.hpp"
    #include "parser.hpp"
    #include "location.hh"

    #include <fmt/format.h>

    #define yyterminate() gq::parser::make_END_OF_INPUT(gq::location());

    #define YY_USER_ACTION wrapper_.increment_location(yyleng);
%}

%option c++ warn nodefault noyywrap
%option yyclass="scanner"
%option prefix="Genquery_"

%%

[ \t\n]                ;
'(''|[^'])*'           { yytext[yyleng - 1] = '\0'; ++yytext; return gq::parser::make_STRING_LITERAL(yytext, gq::location()); }
(?i:select)            return gq::parser::make_SELECT(gq::location());
(?i:where)             return gq::parser::make_WHERE(gq::location());
(?i:like)              return gq::parser::make_LIKE(gq::location());
(?i:in)                return gq::parser::make_IN(gq::location());
(?i:between)           return gq::parser::make_BETWEEN(gq::location());
(?i:no)                return gq::parser::make_NO(gq::location());
(?i:distinct)          return gq::parser::make_DISTINCT(gq::location());
(?i:order)             return gq::parser::make_ORDER(gq::location());
(?i:by)                return gq::parser::make_BY(gq::location());
(?i:asc)               return gq::parser::make_ASC(gq::location());
(?i:desc)              return gq::parser::make_DESC(gq::location());
(?i:offset)            return gq::parser::make_OFFSET(gq::location());
(?i:limit)             return gq::parser::make_LIMIT(gq::location());
(?i:fetch)             return gq::parser::make_FETCH(gq::location());
(?i:first)             return gq::parser::make_FIRST(gq::location());
(?i:rows)              return gq::parser::make_ROWS(gq::location());
(?i:only)              return gq::parser::make_ONLY(gq::location());
(?i:case)              return gq::parser::make_CASE(gq::location());
(?i:when)              return gq::parser::make_WHEN(gq::location());
(?i:else)              return gq::parser::make_ELSE(gq::location());
(?i:end)               return gq::parser::make_END(gq::location());
(?i:group)             return gq::parser::make_GROUP(gq::location());
(?i:having)            return gq::parser::make_HAVING(gq::location());
(?i:exists)            return gq::parser::make_EXISTS(gq::location());
(?i:is)                return gq::parser::make_IS(gq::location());
(?i:null)              return gq::parser::make_NULL(gq::location());
(?i:not)               return gq::parser::make_NOT(gq::location());
(?i:and)               return gq::parser::make_AND(gq::location());
(?i:or)                return gq::parser::make_OR(gq::location());
(?i:cast)              return gq::parser::make_CAST(gq::location());
(?i:as)                return gq::parser::make_AS(gq::location());
"="                    return gq::parser::make_EQUAL(gq::location());
"!="                   return gq::parser::make_NOT_EQUAL(gq::location());
"<>"                   return gq::parser::make_NOT_EQUAL(gq::location());
"<"                    return gq::parser::make_LESS_THAN(gq::location());
"<="                   return gq::parser::make_LESS_THAN_OR_EQUAL_TO(gq::location());
">"                    return gq::parser::make_GREATER_THAN(gq::location());
">="                   return gq::parser::make_GREATER_THAN_OR_EQUAL_TO(gq::location());
","                    return gq::parser::make_COMMA(gq::location());
"("                    return gq::parser::make_PAREN_OPEN(gq::location());
")"                    return gq::parser::make_PAREN_CLOSE(gq::location());
[a-zA-Z][a-zA-Z0-9_]*  return gq::parser::make_IDENTIFIER(yytext, gq::location());
[0-9]+                 return gq::parser::make_POSITIVE_INTEGER(yytext, gq::location());
-?[0-9]+               return gq::parser::make_INTEGER(yytext, gq::location());
.                      throw gq::parser::syntax_error{gq::location(), fmt::format("invalid character: {}", yytext)};
<<EOF>>                return yyterminate();

%%
