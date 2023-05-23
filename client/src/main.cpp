#include "irods/plugins/api/genquery2_common.h"

#include <irods/client_connection.hpp>
#include <irods/irods_exception.hpp>
#include <irods/procApiRequest.h>
#include <irods/rodsClient.h> // For load_client_api_plugins()

#include <fmt/format.h>

#include <cstdlib>

int main(int _argc, char* _argv[]) // NOLINT(modernize-use-trailing-return-type)
{
	if (_argc != 2 && _argc != 3) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		fmt::print("usage: {} [ZONE] QUERY_STRING\n", _argv[0]);
		return 1;
	}

	load_client_api_plugins();

	try {
		irods::experimental::client_connection conn;

                genquery2_input input{};

                if (_argc == 2) {
                    input.query_string = _argv[1];
                }
                else if (_argc == 3) {
                    input.zone = _argv[1];
                    input.query_string = _argv[2];
                }

                char* sql{};

                const auto ec = procApiRequest(
                        static_cast<RcComm*>(conn),
                        IRODS_APN_GENQUERY2,
                        &input,
                        nullptr,
                        reinterpret_cast<void**>(&sql), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                        nullptr);

                if (ec != 0) {
                    fmt::print(stderr, "error: {}\n", ec);
                    return 1;
                }

                fmt::print(fmt::runtime(sql));
                std::free(sql);

		return 0;
	}
	catch (const irods::exception& e) {
		fmt::print(stderr, "error: {}\n", e.client_display_what());
	}
	catch (const std::exception& e) {
		fmt::print(stderr, "error: {}\n", e.what());
	}

	return 1;
}
