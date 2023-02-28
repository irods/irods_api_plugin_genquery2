#include <iostream>

#include "irods/genquery_sql.hpp"
#include "irods/genquery_wrapper.hpp"

int main(int _argc, char* _argv[])
{
    try {
        namespace gq = irods::experimental::api::genquery;
        std::cout << gq::sql(gq::wrapper::parse(_argv[1])) << '\n';
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
