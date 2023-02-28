#include "irods/irods_server_api_call.hpp"
#include "irods/plugins/api/genquery2_common.h"

#include <irods/irods_logger.hpp>
#include <irods/irods_plugin_context.hpp>
#include <irods/irods_re_plugin.hpp>
#include <irods/irods_state_table.h>
#include <irods/rodsError.h>
#include <irods/rodsErrorTable.h>

#include <fmt/format.h>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    // clang-format off
    using log_rule_engine = irods::experimental::log::rule_engine;
    using handler_type    = std::function<irods::error(std::list<boost::any>&, irods::callback&)>;
    // clang-format on

    struct genquery_context
    {
        nlohmann::json rows;
        std::int32_t current_row = -1; 
    }; // struct genquery_context

    std::vector<genquery_context> gq2_context;

    auto get_rei(irods::callback& _effect_handler) -> ruleExecInfo_t&
    {
        ruleExecInfo_t* rei{};
        irods::error result{_effect_handler("unsafe_ms_ctx", &rei)};

        if (!result.ok()) {
            THROW(result.code(), "Failed to get rule execution info");
        }

        return *rei;
    } // get_rei

    auto genquery2_execute(std::list<boost::any>& _rule_arguments, irods::callback& _effect_handler) -> irods::error
    {
        log_rule_engine::info(__func__);
        log_rule_engine::info("number of arguments = [{}]", _rule_arguments.size());

        if (_rule_arguments.size() != 2) {
            const auto msg = fmt::format("Incorrect number of input arguments: expected 2, received {}", _rule_arguments.size());
            log_rule_engine::info(msg);
            return ERROR(SYS_INVALID_INPUT_PARAM, msg);
        }

        try {
            auto& rei = get_rei(_effect_handler);
            auto iter = std::begin(_rule_arguments);
            auto* query = boost::any_cast<std::string*>(*std::next(iter));
            char* results{};

            if (const auto ec = irods::server_api_call(IRODS_APN_GENQUERY2, rei.rsComm, query->c_str(), &results); ec != 0) {
                const auto msg = fmt::format("Error while executing GenQuery 2 query [error_code=[{}]].", ec);
                log_rule_engine::error(msg);
                return ERROR(ec, msg);
            }

            gq2_context.push_back({.rows = nlohmann::json::parse(results), .current_row = -1});
            std::free(results);

            // Return the handle to the caller.
            *boost::any_cast<std::string*>(*iter) = std::to_string(gq2_context.size() - 1);

            return SUCCESS();
        }
        catch (const irods::exception& e) {
            log_rule_engine::error(e.client_display_what());
            return ERROR(e.code(), e.client_display_what());
        }
        catch (const std::exception& e) {
            log_rule_engine::error(e.what());
            return ERROR(SYS_LIBRARY_ERROR, e.what());
        }
    } // genquery2_execute

    auto genquery2_next_row(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
    {
        log_rule_engine::info(__func__);

        if (_rule_arguments.size() != 1) {
            const auto msg = fmt::format("Incorrect number of input arguments: expected 1, received {}", _rule_arguments.size());
            log_rule_engine::info(msg);
            return ERROR(SYS_INVALID_INPUT_PARAM, msg);
        }

        try {
            const auto* ctx_handle = boost::any_cast<std::string*>(*std::begin(_rule_arguments));
            const auto ctx_handle_index = std::stoll(*ctx_handle);

            if (ctx_handle_index < 0 || static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
                constexpr const auto* msg = "Unknown context handle.";
                log_rule_engine::info(msg);
                return ERROR(SYS_INVALID_INPUT_PARAM, msg);
            }

            auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));

            if (ctx.current_row < static_cast<std::int32_t>(ctx.rows.size()) - 1) {
                ++ctx.current_row;
                log_rule_engine::info("Incremented row position [{} => {}]. Returning 0.", ctx.current_row - 1, ctx.current_row);
                return CODE(0);
            }

            log_rule_engine::info("Skipping incrementing of row position [current_row=[{}]]. Returning 1.", ctx.current_row);

            // We must return ERROR(stop_code, "") to trigger correct usage of genquery2_next_row().
            // Otherwise, the NREP can loop forever. Ultimately, this means we aren't allowed to return
            // CODE(stop_code) to signal there is no new row available.
            return ERROR(1, "");
        }
        catch (const irods::exception& e) {
            log_rule_engine::error(e.client_display_what());
            return ERROR(e.code(), e.client_display_what());
        }
        catch (const std::exception& e) {
            log_rule_engine::error(e.what());
            return ERROR(SYS_LIBRARY_ERROR, e.what());
        }

        return SUCCESS();
    } // genquery2_next_row

    auto genquery2_column(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
    {
        log_rule_engine::info(__func__);

        if (_rule_arguments.size() != 3) {
            const auto msg = fmt::format("Incorrect number of input arguments: expected 3, received {}", _rule_arguments.size());
            log_rule_engine::info(msg);
            return ERROR(SYS_INVALID_INPUT_PARAM, msg);
        }

        try {
            auto iter = std::begin(_rule_arguments);
            const auto* ctx_handle = boost::any_cast<std::string*>(*iter);
            const auto ctx_handle_index = std::stoll(*ctx_handle);

            if (ctx_handle_index < 0 || static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
                constexpr const auto* msg = "Unknown context handle.";
                log_rule_engine::info(msg);
                return ERROR(SYS_INVALID_INPUT_PARAM, msg);
            }

            auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));
            iter = std::next(iter);
            const auto column_index = std::stoll(*boost::any_cast<std::string*>(*iter));

            const auto& value = ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();
            log_rule_engine::info("Column value = [{}]", value);
            iter = std::next(iter);
            *boost::any_cast<std::string*>(*iter) = ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();

            return SUCCESS();
        }
        catch (const irods::exception& e) {
            log_rule_engine::error(e.client_display_what());
            return ERROR(e.code(), e.client_display_what());
        }
        catch (const std::exception& e) {
            log_rule_engine::error(e.what());
            return ERROR(SYS_LIBRARY_ERROR, e.what());
        }

        return SUCCESS();
    } // genquery2_column

    auto genquery2_destroy(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
    {
        log_rule_engine::info(__func__);

        if (_rule_arguments.size() != 1) {
            const auto msg = fmt::format("Incorrect number of input arguments: expected 1, received {}", _rule_arguments.size());
            log_rule_engine::info(msg);
            return ERROR(SYS_INVALID_INPUT_PARAM, msg);
        }

        try {
            auto iter = std::begin(_rule_arguments);
            const auto* ctx_handle = boost::any_cast<std::string*>(*iter);
            log_rule_engine::info("ctx_handle = [{}]", *ctx_handle); // FIXME It's empty in the PREP! :-(
            const auto ctx_handle_index = std::stoll(*ctx_handle);

            if (ctx_handle_index < 0 || static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
                constexpr const auto* msg = "Unknown context handle.";
                log_rule_engine::info(msg);
                return ERROR(SYS_INVALID_INPUT_PARAM, msg);
            }

            auto ctx_iter = std::begin(gq2_context);
            std::advance(ctx_iter, ctx_handle_index);
            gq2_context.erase(ctx_iter);

            return SUCCESS();
        }
        catch (const irods::exception& e) {
            log_rule_engine::error(e.client_display_what());
            return ERROR(e.code(), e.client_display_what());
        }
        catch (const std::exception& e) {
            log_rule_engine::error(e.what());
            return ERROR(SYS_LIBRARY_ERROR, e.what());
        }

        return SUCCESS();
    } // genquery2_destroy

    const std::map<std::string_view, handler_type> handlers{
        {"genquery2_execute",  genquery2_execute},
        {"genquery2_next_row", genquery2_next_row},
        {"genquery2_column",   genquery2_column},
        {"genquery2_destroy",  genquery2_destroy}
    };

    //
    // Rule Engine Plugin
    //

    template <typename ...Args>
    using operation = std::function<irods::error(irods::default_re_ctx&, Args...)>;

    auto start(irods::default_re_ctx&, const std::string& _instance_name) -> irods::error
    {
        return SUCCESS();
    } // start

    auto stop(irods::default_re_ctx&, const std::string& _instance_name) -> irods::error
    {
        return SUCCESS();
    } // stop

    auto rule_exists(irods::default_re_ctx&, const std::string& _rule_name, bool& _exists) -> irods::error
    {
        _exists = handlers.find(_rule_name) != std::end(handlers);
        return SUCCESS();
    } // rule_exists

    auto list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules) -> irods::error
    {
        std::transform(std::begin(handlers), std::end(handlers), std::back_inserter(_rules), [](auto _v) {
            return std::string{_v.first};
        });

        return SUCCESS();
    } // list_rules

    auto exec_rule(irods::default_re_ctx&,
                   const std::string& _rule_name,
                   std::list<boost::any>& _rule_arguments,
                   irods::callback _effect_handler) -> irods::error
    {
        log_rule_engine::info(__func__);
        if (const auto iter = handlers.find(_rule_name); iter != std::end(handlers)) {
            return (iter->second)(_rule_arguments, _effect_handler);
        }

        return ERROR(SYS_NOT_SUPPORTED, fmt::format("Rule [{}] not supported.", _rule_name));
    } // exec_rule

#ifdef IRODS_GENQUERY2_USE_RULE_TEXT_WRAPPER
    auto exec_rule_text_impl(std::string_view _rule_text,
                             MsParamArray* _ms_param_array,
                             irods::callback _effect_handler) -> irods::error
    {
        log_rule_engine::debug("_rule_text = [{}]", _rule_text);

        // irule <text>
        if (_rule_text.find("@external rule {") != std::string::npos) {
            const auto start = _rule_text.find_first_of('{') + 1;
            _rule_text = _rule_text.substr(start, _rule_text.rfind(" }") - start);
        }
        // irule -F <script>
        else if (_rule_text.find("@external") != std::string::npos) {
            const auto start = _rule_text.find_first_of('{');
            _rule_text = _rule_text.substr(start, _rule_text.rfind(" }") - start);
        }

        log_rule_engine::info("_rule_text = [{}]", std::string{_rule_text});

        /*
            {
                "op": "execute",
                "query": "select COLL_NAME",
                "args" [strings]
            }

            {
                "op": "next_row",
                "handle": string,
                "args" [strings]
            }

            {
                "op": "column",
                "index": string,
                "value": string,
                "args" [strings]
            }

            {
                "op": "destroy",
                "handle": string,
                "args" [strings]
            }
        */

        using json = nlohmann::json;

        try {
            const auto json_args = json::parse(_rule_text);
            const auto& op = json_args.at("op").get_ref<const std::string&>();

            if (const auto iter = handlers.find(op); iter != std::end(handlers)) {
                std::list<boost::any> args;

                if (op == "genquery2_execute") {
                }
                else if (op == "genquery2_next_row") {
                }
                else if (op == "genquery2_column") {
                }
                else if (op == "genquery2_destroy") {
                }

                return (iter->second)(args, _effect_handler);
            }

            return ERROR(INVALID_OPERATION, fmt::format("Invalid operation [{}]", op));
        }
        catch (const json::exception& e) {
            log_rule_engine::error(e.what());
            return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("Could not parse rule text into JSON object"));
        }
    } // exec_rule_text_impl
#endif // IRODS_GENQUERY2_USE_RULE_TEXT_WRAPPER
} // anonymous namespace

//
// Plugin Factory
//

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

extern "C"
auto plugin_factory(const std::string& _instance_name, const std::string& _context) -> pluggable_rule_engine*
{
    // clang-format off
    [[maybe_unused]] const auto no_op         = [](auto&&...) { return SUCCESS(); };
    [[maybe_unused]] const auto not_supported = [](auto&&...) { return CODE(SYS_NOT_SUPPORTED); };

#ifdef IRODS_GENQUERY2_USE_RULE_TEXT_WRAPPER
    // An adapter that allows exec_rule_text_impl to be used for exec_rule_text operations.
    const auto exec_rule_text_wrapper = [](irods::default_re_ctx& _ctx,
                                           const std::string& _rule_text,
                                           msParamArray_t* _ms_params,
                                           const std::string& _out_desc,
                                           irods::callback _effect_handler)
    {
        return exec_rule_text_impl(_rule_text, _ms_params, _effect_handler);
    };

    // An adapter that allows exec_rule_text_impl to be used for exec_rule_expression operations.
    const auto exec_rule_expression_wrapper = [](irods::default_re_ctx& _ctx,
                                                 const std::string& _rule_text,
                                                 msParamArray_t* _ms_params,
                                                 irods::callback _effect_handler)
    {
        return exec_rule_text_impl(_rule_text, _ms_params, _effect_handler);
    };
    // clang-format on
#endif

    auto* re = new pluggable_rule_engine{_instance_name, _context};

    re->add_operation("start", operation<const std::string&>{start});
    re->add_operation("stop", operation<const std::string&>{stop});
    re->add_operation("rule_exists", operation<const std::string&, bool&>{rule_exists});
    re->add_operation("list_rules", operation<std::vector<std::string>&>{list_rules});
    re->add_operation("exec_rule", operation<const std::string&, std::list<boost::any>&, irods::callback>{exec_rule});

    // Replace the "exec_rule_*_wrapper" arguments with the "not_supported" lambda if support for
    // "irule" is not needed.
#ifdef IRODS_GENQUERY2_USE_RULE_TEXT_WRAPPER
    re->add_operation("exec_rule_text", operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{exec_rule_text_wrapper});
    re->add_operation("exec_rule_expression", operation<const std::string&, msParamArray_t*, irods::callback>{exec_rule_expression_wrapper});
#else
    re->add_operation("exec_rule_text", operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{not_supported});
    re->add_operation("exec_rule_expression", operation<const std::string&, msParamArray_t*, irods::callback>{not_supported});
#endif

    return re;
}
