%skeleton "lalr1.cc" /* Enables C++ */
%require "3.0" /* position.hh and stack.hh are deprecated in 3.2 and later. */
%language "C++" /* Redundant? */
%defines /* Doesn't work in version 3.0 - %header */

/* The name of the parser class. Defaults to "parser" in C++. */
/* %define api.parser.class { parser } */

/*
Request that symbols be handled as a whole (type, value, and possibly location)
in the scanner. In C++, only works when variant-based semantic values are enabled.
This option causes make_* functions to be generated for each token kind.
*/
%define api.token.constructor 

/* The type used for semantic values. */
%define api.value.type variant

/* Enables runtime assertions to catch invalid uses. In C++, detects improper use of variants. */
%define parse.assert

/* Defines the namespace for the parser class. */
%define api.namespace { irods::experimental::api::genquery }

%code top
{
    #include "irods/genquery_scanner.hpp"
    #include "parser.hpp"
    #include "irods/genquery_wrapper.hpp"
    #include "location.hh"

    static gq::parser::symbol_type yylex(gq::scanner& _scanner, gq::wrapper& _wrapper)
    {
        return _scanner.get_next_token();
    }
}

%code requires
{
    #include "irods/genquery_ast_types.hpp"

    #include <string>
    #include <vector>

    #include <fmt/format.h>

    namespace irods::experimental::api::genquery
    {
        class scanner;
        class wrapper;
    } // namespace irods::experimental::api::genquery

    namespace gq = irods::experimental::api::genquery;
}

/*
The following can be replaced by %param.
%lex-param { gq::scanner& scanner } { gq::wrapper& wrapper }
%parse-param { gq::scanner& scanner } { gq::wrapper& wrapper }
*/
%param { gq::scanner& scanner } { gq::wrapper& wrapper }
%locations
%define parse.trace
%define parse.error verbose /* Can produce incorrect info if LAC is not enabled. */

%define api.token.prefix {IRODS_GENQUERY_TOKEN_}

%token <std::string> IDENTIFIER STRING_LITERAL POSITIVE_INTEGER INTEGER
%token SELECT NO DISTINCT WHERE NOT AND OR COMMA PAREN_OPEN PAREN_CLOSE
%token BETWEEN EQUAL NOT_EQUAL LIKE IN
%token LESS_THAN GREATER_THAN LESS_THAN_OR_EQUAL_TO GREATER_THAN_OR_EQUAL_TO
%token ORDER BY ASC DESC
%token OFFSET FETCH FIRST ROWS ONLY LIMIT
%token CASE WHEN ELSE END
%token GROUP HAVING EXISTS IS NULL
%token CAST AS
%token END_OF_INPUT 0

/*
%left is the same as %token, but defines associativity.
%token does not define associativity or precedence.
%precedence does not define associativity.

Operator precedence is determined by the line ordering of the declarations. 
The further down the page the line is, the higher the precedence. For example,
NOT has higher precedence than AND.

While these directives support specifying a semantic type, Bison recommends not
doing that and using these directives to specify precedence and associativity
rules only.
*/
%left OR
%left AND
%precedence NOT

%type<gq::selections> selections;
%type<gq::conditions> conditions;
%type<gq::order_by> order_by;
%type<std::vector<gq::sort_expression>> sort_expr;
%type<gq::range> range;
%type<gq::selection> selection;
%type<gq::column> column;
%type<gq::select_function> select_function;
%type<gq::condition> condition;
%type<gq::condition_expression> condition_expression;
%type<std::vector<std::string>> list_of_string_literals;
%type<std::vector<std::string>> list_of_identifiers;

%start genquery /* Defines where grammar starts */

%%

genquery:
    select
  | select order_by  { std::swap(wrapper.select_.order_by, $2); }
  | select range  { std::swap(wrapper.select_.range, $2); }
  | select order_by range  { std::swap(wrapper.select_.order_by, $2); std::swap(wrapper.select_.range, $3); }
  | select range order_by  { std::swap(wrapper.select_.order_by, $3); std::swap(wrapper.select_.range, $2); }

select:
    SELECT selections  { std::swap(wrapper.select_.selections, $2); }
  | SELECT selections WHERE conditions  { std::swap(wrapper.select_.selections, $2); std::swap(wrapper.select_.conditions, $4); }
  | SELECT NO DISTINCT selections  { wrapper.select_.distinct = false; std::swap(wrapper.select_.selections, $4); }
  | SELECT NO DISTINCT selections WHERE conditions  { wrapper.select_.distinct = false; std::swap(wrapper.select_.selections, $4); std::swap(wrapper.select_.conditions, $6); }

order_by:
    ORDER BY sort_expr  { std::swap($$.sort_expressions, $3); }

sort_expr:
    IDENTIFIER  { $$.push_back(gq::sort_expression{$1, true}); }
  | IDENTIFIER ASC  { $$.push_back(gq::sort_expression{$1, true}); }
  | IDENTIFIER DESC  { $$.push_back(gq::sort_expression{$1, false}); }
  | sort_expr COMMA IDENTIFIER  { $1.push_back(gq::sort_expression{$3, true}); std::swap($$, $1); }
  | sort_expr COMMA IDENTIFIER ASC  { $1.push_back(gq::sort_expression{$3, true}); std::swap($$, $1); }
  | sort_expr COMMA IDENTIFIER DESC  { $1.push_back(gq::sort_expression{$3, false}); std::swap($$, $1); }

range:
    OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $2); }
  | OFFSET POSITIVE_INTEGER FETCH FIRST POSITIVE_INTEGER ROWS ONLY  { std::swap($$.offset, $2); std::swap($$.number_of_rows, $5); }
  | OFFSET POSITIVE_INTEGER LIMIT POSITIVE_INTEGER  { std::swap($$.offset, $2); std::swap($$.number_of_rows, $4); }
  | FETCH FIRST POSITIVE_INTEGER ROWS ONLY  { std::swap($$.number_of_rows, $3); }
  | FETCH FIRST POSITIVE_INTEGER ROWS ONLY OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $7); std::swap($$.number_of_rows, $3); }
  | LIMIT POSITIVE_INTEGER  { std::swap($$.number_of_rows, $2); }
  | LIMIT POSITIVE_INTEGER OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $4); std::swap($$.number_of_rows, $2); }

selections:
    selection  { $$ = gq::selections{std::move($1)}; }
  | selections COMMA selection  { $1.push_back(std::move($3)); std::swap($$, $1); }

selection:
    column  { $$ = std::move($1); }
  | select_function  { $$ = std::move($1); }

column:
    IDENTIFIER  { $$ = gq::column{std::move($1)}; }
  | CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_CLOSE  { $$ = gq::column{$3, $5}; }
  | CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_OPEN POSITIVE_INTEGER PAREN_CLOSE PAREN_CLOSE  { $$ = gq::column{$3, fmt::format("{}({})", $5, $7)}; }

select_function:
    IDENTIFIER PAREN_OPEN column PAREN_CLOSE  { $$ = gq::select_function{std::move($1), gq::column{std::move($3)}}; }
    /*IDENTIFIER PAREN_OPEN IDENTIFIER PAREN_CLOSE  { $$ = gq::select_function{std::move($1), gq::column{std::move($3)}}; }*/

conditions:
    condition  { $$ = gq::conditions{std::move($1)}; }
  | conditions AND conditions  { $1.push_back(gq::logical_and{std::move($3)}); std::swap($$, $1); }
  | conditions OR conditions  { $1.push_back(gq::logical_or{std::move($3)}); std::swap($$, $1); }
  | PAREN_OPEN conditions PAREN_CLOSE  { $$ = gq::conditions{gq::logical_grouping{std::move($2)}}; }
  | NOT conditions  { $$ = gq::conditions{gq::logical_not{std::move($2)}}; }

condition:
    column condition_expression  { $$ = gq::condition(std::move($1), std::move($2)); }

condition_expression:
    LIKE STRING_LITERAL  { $$ = gq::condition_like(std::move($2)); }
  | NOT LIKE STRING_LITERAL  { $$ = gq::condition_operator_not{gq::condition_like(std::move($3))}; }
  | IN PAREN_OPEN list_of_string_literals PAREN_CLOSE  { $$ = gq::condition_in(std::move($3)); }
  | NOT IN PAREN_OPEN list_of_string_literals PAREN_CLOSE  { $$ = gq::condition_operator_not{gq::condition_in(std::move($4))}; }
  | BETWEEN STRING_LITERAL AND STRING_LITERAL  { $$ = gq::condition_between(std::move($2), std::move($4)); }
  | NOT BETWEEN STRING_LITERAL AND STRING_LITERAL  { $$ = gq::condition_operator_not{gq::condition_between(std::move($3), std::move($5))}; }
  | EQUAL STRING_LITERAL  { $$ = gq::condition_equal(std::move($2)); }
  | NOT_EQUAL STRING_LITERAL  { $$ = gq::condition_not_equal(std::move($2)); }
  | LESS_THAN STRING_LITERAL  { $$ = gq::condition_less_than(std::move($2)); }
  | LESS_THAN_OR_EQUAL_TO STRING_LITERAL  { $$ = gq::condition_less_than_or_equal_to(std::move($2)); }
  | GREATER_THAN STRING_LITERAL  { $$ = gq::condition_greater_than(std::move($2)); }
  | GREATER_THAN_OR_EQUAL_TO STRING_LITERAL  { $$ = gq::condition_greater_than_or_equal_to(std::move($2)); }
  | IS NULL  { $$ = gq::condition_is_null{}; }
  | IS NOT NULL  { $$ = gq::condition_is_not_null{}; }

list_of_string_literals:
    STRING_LITERAL  { $$ = std::vector<std::string>{std::move($1)}; }
  | list_of_string_literals COMMA STRING_LITERAL  { $1.push_back(std::move($3)); std::swap($$, $1); }

list_of_identifiers:
    IDENTIFIER  { $$ = std::vector<std::string>{std::move($1)}; }
  | list_of_identifiers COMMA IDENTIFIER  { $1.push_back(std::move($3)); std::swap($$, $1); }

%%

void gq::parser::error(const location& _location, const std::string& _message)
{
    throw std::invalid_argument{fmt::format("{} @ {}", _message, wrapper.location())};
} // gq::parser::error
