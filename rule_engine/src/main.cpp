#include "irods/irods_server_api_call.hpp"
#include "irods/plugins/api/genquery2_common.h"

#include <irods/irods_at_scope_exit.hpp>
#include <irods/irods_plugin_context.hpp>
#include <irods/irods_re_plugin.hpp>
#include <irods/irods_state_table.h>
#include <irods/rodsError.h>
#include <irods/rodsErrorTable.h>

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
#  include <irods/rodsLog.h>
#  include <json.hpp>
#else
#  include <irods/irods_logger.hpp>
#  include <nlohmann/json.hpp>
#endif // IRODS_ENABLE_42X_COMPATIBILITY

#include <fmt/format.h>
#include <boost/any.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// clang-format off
#ifndef IRODS_ENABLE_42X_COMPATIBILITY
	using log_re = irods::experimental::log::rule_engine;
#endif // IRODS_ENABLE_42X_COMPATIBILITY
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
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG8, __func__);
		rodsLog(LOG_DEBUG8, fmt::format("Number of arguments = [{}]", _rule_arguments.size()).c_str());
#else
		log_re::trace(__func__);
		log_re::debug("Number of arguments = [{}]", _rule_arguments.size());
#endif // IRODS_ENABLE_42X_COMPATIBILITY

		if (_rule_arguments.size() != 2) {
			const auto msg =
				fmt::format("Incorrect number of input arguments: expected 2, received {}", _rule_arguments.size());
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, msg.c_str());
#else
			log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_INVALID_INPUT_PARAM, msg);
		}

		try {
			auto& rei = get_rei(_effect_handler);
			auto iter = std::begin(_rule_arguments);
			auto* query = boost::any_cast<std::string*>(*std::next(iter));

			genquery2_input input{};
			input.query_string = strdup(query->c_str());
			irods::at_scope_exit free_input_struct{[&input] { std::free(input.query_string); }};

			char* results{};

			if (const auto ec = irods::server_api_call(IRODS_APN_GENQUERY2, rei.rsComm, &input, &results); ec != 0) {
				const auto msg = fmt::format("Error while executing GenQuery2 query [error_code=[{}]].", ec);
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, msg.c_str());
#else
				log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return ERROR(ec, msg);
			}

			gq2_context.push_back({.rows = nlohmann::json::parse(results), .current_row = -1});
			std::free(results);

			// Return the handle to the caller.
			*boost::any_cast<std::string*>(*iter) = std::to_string(gq2_context.size() - 1);

			return SUCCESS();
		}
		catch (const irods::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.client_display_what());
#else
			log_re::error(e.client_display_what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(e.code(), e.client_display_what());
		}
		catch (const std::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.what());
#else
			log_re::error(e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_LIBRARY_ERROR, e.what());
		}
	} // genquery2_execute

	auto genquery2_next_row(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
	{
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG8, __func__);
#else
		log_re::trace(__func__);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

		if (_rule_arguments.size() != 1) {
			const auto msg =
				fmt::format("Incorrect number of input arguments: expected 1, received {}", _rule_arguments.size());
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, msg.c_str());
#else
			log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_INVALID_INPUT_PARAM, msg);
		}

		try {
			const auto* ctx_handle = boost::any_cast<std::string*>(*std::begin(_rule_arguments));
			const auto ctx_handle_index = std::stoll(*ctx_handle);

			if (ctx_handle_index < 0 ||
			    static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
				constexpr const auto* msg = "Unknown context handle.";
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, msg);
#else
				log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return ERROR(SYS_INVALID_INPUT_PARAM, msg);
			}

			auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));

			if (ctx.current_row < static_cast<std::int32_t>(ctx.rows.size()) - 1) {
				++ctx.current_row;
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				const auto msg = fmt::format(
					"Incremented row position [{} => {}]. Returning 0.", ctx.current_row - 1, ctx.current_row);
				rodsLog(LOG_DEBUG8, msg.c_str());
#else
				log_re::trace(
					"Incremented row position [{} => {}]. Returning 0.", ctx.current_row - 1, ctx.current_row);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return CODE(0);
			}

#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG8, "Skipping increment of row position [current_row=[%d]]. Returning 1.", ctx.current_row);
#else
			log_re::trace("Skipping increment of row position [current_row=[{}]]. Returning 1.", ctx.current_row);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

			// We must return ERROR(stop_code, "") to trigger correct usage of genquery2_next_row().
			// Otherwise, the NREP can loop forever. Ultimately, this means we aren't allowed to return
			// CODE(stop_code) to signal there is no new row available.
			return ERROR(1, "");
		}
		catch (const irods::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.client_display_what());
#else
			log_re::error(e.client_display_what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(e.code(), e.client_display_what());
		}
		catch (const std::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.what());
#else
			log_re::error(e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_LIBRARY_ERROR, e.what());
		}

		return SUCCESS();
	} // genquery2_next_row

	auto genquery2_column(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
	{
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG8, __func__);
#else
		log_re::trace(__func__);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

		if (_rule_arguments.size() != 3) {
			const auto msg =
				fmt::format("Incorrect number of input arguments: expected 3, received {}", _rule_arguments.size());
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, msg.c_str());
#else
			log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_INVALID_INPUT_PARAM, msg);
		}

		try {
			auto iter = std::begin(_rule_arguments);
			const auto* ctx_handle = boost::any_cast<std::string*>(*iter);
			const auto ctx_handle_index = std::stoll(*ctx_handle);

			if (ctx_handle_index < 0 ||
			    static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
				constexpr const auto* msg = "Unknown context handle.";
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, msg);
#else
				log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return ERROR(SYS_INVALID_INPUT_PARAM, msg);
			}

			auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));
			iter = std::next(iter);
			const auto column_index = std::stoll(*boost::any_cast<std::string*>(*iter));

			const auto& value = ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG, "Column value = [%s]", value.c_str());
#else
			log_re::debug("Column value = [{}]", value);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			iter = std::next(iter);
			*boost::any_cast<std::string*>(*iter) =
				ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();

			return SUCCESS();
		}
		catch (const irods::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.client_display_what());
#else
			log_re::error(e.client_display_what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(e.code(), e.client_display_what());
		}
		catch (const std::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.what());
#else
			log_re::error(e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_LIBRARY_ERROR, e.what());
		}

		return SUCCESS();
	} // genquery2_column

	auto genquery2_destroy(std::list<boost::any>& _rule_arguments, irods::callback&) -> irods::error
	{
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG8, __func__);
#else
		log_re::trace(__func__);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

		if (_rule_arguments.size() != 1) {
			const auto msg =
				fmt::format("Incorrect number of input arguments: expected 1, received {}", _rule_arguments.size());
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, msg.c_str());
#else
			log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_INVALID_INPUT_PARAM, msg);
		}

		try {
			auto iter = std::begin(_rule_arguments);
			const auto* ctx_handle = boost::any_cast<std::string*>(*iter);
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_DEBUG8, "ctx_handle = [%s]", ctx_handle->c_str());
#else
			log_re::trace("ctx_handle = [{}]", *ctx_handle);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			const auto ctx_handle_index = std::stoll(*ctx_handle);

			if (ctx_handle_index < 0 ||
			    static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
				constexpr const auto* msg = "Unknown context handle.";
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
				rodsLog(LOG_ERROR, msg);
#else
				log_re::error(msg);
#endif // IRODS_ENABLE_42X_COMPATIBILITY
				return ERROR(SYS_INVALID_INPUT_PARAM, msg);
			}

			auto ctx_iter = std::begin(gq2_context);
			std::advance(ctx_iter, ctx_handle_index);
			gq2_context.erase(ctx_iter);

			return SUCCESS();
		}
		catch (const irods::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.client_display_what());
#else
			log_re::error(e.client_display_what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(e.code(), e.client_display_what());
		}
		catch (const std::exception& e) {
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.what());
#else
			log_re::error(e.what());
#endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_LIBRARY_ERROR, e.what());
		}

		return SUCCESS();
	} // genquery2_destroy

	// clang-format off
	const std::map<std::string_view, handler_type> handlers{
		{"genquery2_execute", genquery2_execute},
		{"genquery2_next_row", genquery2_next_row},
		{"genquery2_column", genquery2_column},
		{"genquery2_destroy", genquery2_destroy}
	};
	// clang-format on

	//
	// Rule Engine Plugin
	//

	template <typename... Args>
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
#ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG8, __func__);
#else
		log_re::trace(__func__);
#endif // IRODS_ENABLE_42X_COMPATIBILITY

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
#  ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG, "_rule_text = [%s]", std::string{_rule_text}.c_str());
#  else
		log_re::debug("_rule_text = [{}]", _rule_text);
#  endif // IRODS_ENABLE_42X_COMPATIBILITY

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

#  ifdef IRODS_ENABLE_42X_COMPATIBILITY
		rodsLog(LOG_DEBUG, "_rule_text = [%s]", std::string{_rule_text}.c_str());
#  else
		log_re::debug("_rule_text = [{}]", std::string{_rule_text});
#  endif // IRODS_ENABLE_42X_COMPATIBILITY

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
#  ifdef IRODS_ENABLE_42X_COMPATIBILITY
			rodsLog(LOG_ERROR, e.what());
#  else
			log_re::error(e.what());
#  endif // IRODS_ENABLE_42X_COMPATIBILITY
			return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("Could not parse rule text into JSON object"));
		}
	}  // exec_rule_text_impl
#endif // IRODS_GENQUERY2_USE_RULE_TEXT_WRAPPER
} // anonymous namespace

//
// Plugin Factory
//

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

extern "C" auto plugin_factory(const std::string& _instance_name, const std::string& _context) -> pluggable_rule_engine*
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
	re->add_operation(
		"exec_rule_text",
		operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{exec_rule_text_wrapper});
	re->add_operation("exec_rule_expression",
	                  operation<const std::string&, msParamArray_t*, irods::callback>{exec_rule_expression_wrapper});
#else
	re->add_operation(
		"exec_rule_text",
		operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{not_supported});
	re->add_operation(
		"exec_rule_expression", operation<const std::string&, msParamArray_t*, irods::callback>{not_supported});
#endif

	return re;
} // plugin_factory
