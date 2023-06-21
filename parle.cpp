/*
 * Copyright (c) 2022 Anatol Belski
 * All rights reserved.
 *
 * Author: Anatol Belski <ab@php.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS
#include "inttypes.h"

#include "include/lexertl/enum_operator.hpp"
#include "include/lexertl/generator.hpp"
#include "include/lexertl/lookup.hpp"
#include "include/lexertl/iterator.hpp"
#include "include/lexertl/debug.hpp"
#include "include/lexertl/match_results.hpp"
#include "include/lexertl/state_machine.hpp"
#include "include/lexertl/utf_iterators.hpp"

#include "include/parsertl/generator.hpp"
#include "include/parsertl/lookup.hpp"
#include "include/parsertl/read_bison.hpp"
#include "include/parsertl/state_machine.hpp"
#include "include/parsertl/parse.hpp"
#include "include/parsertl/token.hpp"
#include "include/parsertl/debug.hpp"
#include "include/parsertl/rules.hpp"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "php_parle.h"

#ifdef HAVE_PARLE_UTF32
#define PARLE_U32 1
#else
#define PARLE_U32 0
#endif

/* {{{ Class entries and handlers declarations. */
zend_object_handlers parle_lexer_handlers;
zend_object_handlers parle_rlexer_handlers;
zend_object_handlers parle_parser_handlers;
zend_object_handlers parle_rparser_handlers;
zend_object_handlers parle_stack_handlers;

static zend_class_entry *ParleLexer_ce;
static zend_class_entry *ParleRLexer_ce;
static zend_class_entry *ParleParser_ce;
static zend_class_entry *ParleRParser_ce;
static zend_class_entry *ParleStack_ce;
static zend_class_entry *ParleLexerException_ce;
static zend_class_entry *ParleParserException_ce;
static zend_class_entry *ParleStackException_ce;
static zend_class_entry *ParleToken_ce;
static zend_class_entry *ParleErrorInfo_ce;
/* }}} */

namespace parle {/*{{{*/
	using id_type = uint16_t;
#if PARLE_U32
#if defined(_MSC_VER)
	// dirty quirk
	using char_type = uint32_t;
	using string = std::basic_string<char_type>;
#if PARLE_U32
	using utf_out_iter = lexertl::basic_utf8_out_iterator
		<const parle::char_type*>;
#endif
#else
	using char_type = char32_t;
	using string = std::u32string;
#if PARLE_U32
	using utf_out_iter = lexertl::basic_utf8_out_iterator
		<const parle::char_type*>;
#endif
#endif
#else
	using char_type = char;
	using string = std::string;
#endif
}/* }}} */

#include "parle/cvt.hpp"
#include "parle/lexer/iterator.hpp"

#undef lookup

namespace parle {/*{{{*/
	namespace parser {
		struct parser;
		struct rparser;
	}

	namespace lexer {
		struct token_cb {
			zval cb;
		};

		struct lexer;
		struct rlexer;

		using state_machine = lexertl::basic_state_machine<char_type, id_type>;
		using parle_rules = lexertl::basic_rules<char_type, char_type, id_type>;

		using cmatch = lexertl::match_results<const char_type *, id_type>;
		using crmatch = lexertl::recursive_match_results<const char_type *, id_type>;
		using citerator = iterator<const char_type *, state_machine, cmatch, lexer, token_cb, id_type>;
		using criterator = iterator<const char_type *, state_machine, crmatch, rlexer, token_cb, id_type>;

		using smatch = lexertl::match_results<string::const_iterator, id_type>;
		using srmatch = lexertl::recursive_match_results<string::const_iterator, id_type>;
		using siterator = iterator<string::const_iterator, state_machine, smatch, lexer, token_cb, id_type>;
		using sriterator = iterator<string::const_iterator, state_machine, srmatch, rlexer, token_cb, id_type>;

		using generator = lexertl::basic_generator<parle_rules, state_machine>;
		using debug = lexertl::basic_debug<state_machine, char_type, id_type>;

		struct lexer {
			lexer() : in(PARLE_PRE_U32("")), par(nullptr) {}
			string in;
			parle_rules rules;
			state_machine sm;
			parle::parser::parser *par;
			siterator iter;
			siterator::cb_map cb_map;
		};

		struct rlexer {
			rlexer() : in(PARLE_PRE_U32("")), par(nullptr) {}
			string in;
			parle_rules rules;
			state_machine sm;
			parle::parser::rparser *par;
			sriterator iter;
			sriterator::cb_map cb_map;
		};
	}

	namespace parser {
		using state_machine = parsertl::basic_state_machine<id_type>;
		using match_results = parsertl::basic_match_results<state_machine>;
		using parle_rules = parsertl::basic_rules<char_type, id_type>;
		using generator = parsertl::basic_generator<parle_rules, state_machine, id_type>;
		using parle_productions = parsertl::token<parle::lexer::siterator>::token_vector;
		using parle_rproductions = parsertl::token<parle::lexer::sriterator>::token_vector;
		using debug = parsertl::basic_debug<char_type>;

		struct parser {
			parser() : lex(nullptr) {}
			parle_rules rules;
			state_machine sm;
			match_results results;
			parle::lexer::lexer *lex;
			parle_productions productions;
		};

		struct rparser {
			rparser() : lex(nullptr) {}
			parle_rules rules;
			state_machine sm;
			match_results results;
			parle::lexer::rlexer *lex;
			parle_rproductions productions;
		};
	}

	namespace stack {
		using stack = std::stack<zval *>;
	}
}/*}}}*/

/* If you declare any globals in php_parle.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(parle)
*/

/* True global resources - no need for thread safety here */
/* static int le_parle; */

struct ze_parle_lexer_obj {/*{{{*/
	parle::lexer::lexer *lex;
	zend_object zo;
};/*}}}*/

struct ze_parle_rlexer_obj {/*{{{*/
	parle::lexer::rlexer *lex;
	zend_object zo;
};/*}}}*/

struct ze_parle_parser_obj {/*{{{*/
	parle::parser::parser *par;
	zend_object zo;
};/*}}}*/

struct ze_parle_rparser_obj {/*{{{*/
	parle::parser::rparser *par;
	zend_object zo;
};/*}}}*/

struct ze_parle_stack_obj {/*{{{*/
	parle::stack::stack *stack;
	zend_object zo;
};/*}}}*/


template<typename lexer_obj_type> lexer_obj_type *
_php_parle_lexer_fetch_zobj(zend_object *obj) noexcept
{/*{{{*/
	return (lexer_obj_type *)((char *)obj - XtOffsetOf(lexer_obj_type, zo));
}/*}}}*/

static zend_always_inline ze_parle_lexer_obj *
php_parle_lexer_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return _php_parle_lexer_fetch_zobj<ze_parle_lexer_obj>(obj);
}/*}}}*/

static zend_always_inline ze_parle_rlexer_obj *
php_parle_rlexer_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return _php_parle_lexer_fetch_zobj<ze_parle_rlexer_obj>(obj);
}/*}}}*/

template<typename parser_obj_type> parser_obj_type *
_php_parle_parser_fetch_zobj(zend_object *obj) noexcept
{/*{{{*/
	return (parser_obj_type *)((char *)obj - XtOffsetOf(parser_obj_type, zo));
}/*}}}*/

static zend_always_inline ze_parle_parser_obj *
php_parle_parser_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return _php_parle_parser_fetch_zobj<ze_parle_parser_obj>(obj);
}/*}}}*/

static zend_always_inline ze_parle_rparser_obj *
php_parle_rparser_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return _php_parle_parser_fetch_zobj<ze_parle_rparser_obj>(obj);
}/*}}}*/

static zend_always_inline ze_parle_stack_obj *
php_parle_stack_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return (ze_parle_stack_obj *)((char *)obj - XtOffsetOf(ze_parle_stack_obj, zo));
}/*}}}*/

static void
php_parle_rethrow_from_cpp(zend_class_entry *ce, const char *msg, zend_long code)
{/*{{{*/
#if 0
	const char *m = estrdup(msg);
	zend_throw_exception_ex(ce, code, "%s", m);
	efree((void *)m);
#endif
	zend_throw_exception_ex(ce, code, "%s", msg);
}/*}}}*/

/* {{{ public void Lexer::push(...) */
PHP_METHOD(ParleLexer, push)
{
	ze_parle_lexer_obj *zplo;
	zend_string *regex;
	zend_long id, user_id = -1;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSl|l", &me, ParleLexer_ce, &regex, &id, &user_id) == FAILURE) {
		return;
	}

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));

	try {
		// Rules for INITIAL
		auto &lex = *zplo->lex;
		if (user_id < 0) user_id = lex.iter->npos();
		lex.rules.push(PARLE_CVT_U32(ZSTR_VAL(regex)), static_cast<parle::id_type>(id), static_cast<parle::id_type>(user_id));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void RLexer::push(...) */
PHP_METHOD(ParleRLexer, push)
{
#define PREPARE_PUSH() \
	zplo = php_parle_rlexer_fetch_obj(Z_OBJ_P(me)); \
	auto &lex = *zplo->lex; \
	if (user_id < 0) user_id = lex.iter->npos();

	try {
		ze_parle_rlexer_obj *zplo;
		zend_string *regex, *dfa, *new_dfa;
		zend_long id, user_id = -1;
		zval *me;

		// Rules for INITIAL
		if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSl|l", &me, ParleRLexer_ce, &regex, &id, &user_id) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(PARLE_CVT_U32(ZSTR_VAL(regex)), static_cast<parle::id_type>(id), static_cast<parle::id_type>(user_id));
		// Rules with id
		} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSlS|l", &me, ParleRLexer_ce, &dfa, &regex, &id, &new_dfa, &user_id) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(PARLE_CVT_U32(ZSTR_VAL(dfa)), PARLE_CVT_U32(ZSTR_VAL(regex)), static_cast<parle::id_type>(id), PARLE_CVT_U32(ZSTR_VAL(new_dfa)), static_cast<parle::id_type>(user_id));
		// Rules without id
		} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSS", &me, ParleRLexer_ce, &dfa, &regex, &new_dfa) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(PARLE_CVT_U32(ZSTR_VAL(dfa)), PARLE_CVT_U32(ZSTR_VAL(regex)), PARLE_CVT_U32(ZSTR_VAL(new_dfa)));
		} else {
			zend_throw_exception(ParleLexerException_ce, "Couldn't match the method signature", 0);
		}
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
#undef PREPARE_PUSH
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_callout(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me, *cb;
	parle::lexer::token_cb tcb;
	zend_long id;
	zend_string *cb_name;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Olz", &me, ce, &id, &cb) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	if (!zend_is_callable(cb, 0, &cb_name)) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "%s is not callable", ZSTR_VAL(cb_name));
		zend_string_release(cb_name);
		return;
	}
	zend_string_release(cb_name);

	ZVAL_COPY(&tcb.cb, cb);
	lex.cb_map.emplace(static_cast<parle::id_type>(id), std::move(tcb));
}/*}}}*/

/* {{{ public void Lexer::callout(integer $id, callable $callback) */
PHP_METHOD(ParleLexer, callout)
{
	_lexer_callout<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::callout(integer $id, callable $callback) */
PHP_METHOD(ParleRLexer, callout)
{
	_lexer_callout<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

/* }}} */
template<typename lexer_obj_type> void
_lexer_build(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	try {
		parle::lexer::generator::build(lex.rules, lex.sm);
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::build(void) */
PHP_METHOD(ParleLexer, build)
{
	_lexer_build<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::build(void) */
PHP_METHOD(ParleRLexer, build)
{
	_lexer_build<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_consume(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	char *in;
	size_t in_len;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &me, ce, &in, &in_len) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	try {
		lex.in = PARLE_CVT_U32(in);
		lex.iter = {lex.in.begin(), lex.in.end(), lex};
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::consume(string $s) */
PHP_METHOD(ParleLexer, consume)
{
	_lexer_consume<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::consume(string $s) */
PHP_METHOD(ParleRLexer, consume)
{
	_lexer_consume<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

/* {{{ public int RLexer::pushState(string $s) */
PHP_METHOD(ParleRLexer, pushState)
{
	ze_parle_rlexer_obj *zplo;
	char *state;
	size_t state_len;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &me, ParleRLexer_ce, &state, &state_len) == FAILURE) {
		return;
	}

	zplo = php_parle_rlexer_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zplo->lex->rules;
		RETURN_LONG(rules.push_state(PARLE_CVT_U32(state)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_token(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l", &me, ce) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	try {
		object_init_ex(return_value, ParleToken_ce);
		std::string ret{PARLE_SCVT_U8(lex.iter->str())};
		add_property_long_ex(return_value, "id", sizeof("id")-1, static_cast<zend_long>(lex.iter->id));
#if PHP_MAJOR_VERSION > 7 || PHP_MAJOR_VERSION >= 7 && PHP_MINOR_VERSION >= 2
		add_property_stringl_ex(return_value, "value", sizeof("value")-1, ret.c_str(), ret.size());
#else
		add_property_stringl_ex(return_value, "value", sizeof("value")-1, (char *)ret.c_str(), ret.size());
#endif

	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public Lexer\Token Lexer::getToken(void) */
PHP_METHOD(ParleLexer, getToken)
{
	_lexer_token<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public Lexer\Token RLexer::getToken(void) */
PHP_METHOD(ParleRLexer, getToken)
{
	_lexer_token<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_advance(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	try {
		auto &lex = *zplo->lex;
		if (lex.iter->first == lex.iter->eoi) {
			return;
		}
		lex.iter++;
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::advance(void) */
PHP_METHOD(ParleLexer, advance)
{
	_lexer_advance<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::advance(void) */
PHP_METHOD(ParleRLexer, advance)
{
	_lexer_advance<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_reset(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;
	zend_long pos;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol", &me, ce, &pos) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	if (pos < 0 || static_cast<size_t>(pos) > lex.in.length()) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "Invalid offset " ZEND_LONG_FMT, pos);
		return;
	}

	try {
		//if (lex.par) {
			/* Don't replace iter as it's passed to the parser already.*/
			lex.iter.reset(lex.in.begin() + static_cast<size_t>(pos), lex.in.end());
		//} else {
		//	lex.iter = {lex.in.begin() + static_cast<size_t>(pos), lex.in.end(), lex};
		//}
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::reset(int $position) */
PHP_METHOD(ParleLexer, reset)
{
	_lexer_reset<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::reset(int $position) */
PHP_METHOD(ParleRLexer, reset)
{
	_lexer_reset<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_macro(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;
	zend_string *name, *regex;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSS", &me, ce, &name, &regex) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	try {
		auto &lex = *zplo->lex;
		lex.rules.insert_macro(PARLE_CVT_U32(ZSTR_VAL(name)), PARLE_CVT_U32(ZSTR_VAL(regex)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::insertMacro(string $name, string $reg) */
PHP_METHOD(ParleLexer, insertMacro)
{
	_lexer_macro<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::insertMacro(string $name, string $reg) */
PHP_METHOD(ParleRLexer, insertMacro)
{
	_lexer_macro<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_dump(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	try {
#if PARLE_U32
		std::basic_stringstream<parle::char_type> ss;
		parle::string str;

		parle::lexer::debug::dump(lex.sm, lex.rules, ss);
		str = ss.str();

		const parle::char_type* end_str = str.c_str() + str.size();
		parle::utf_out_iter iter(str.c_str(), end_str);
		parle::utf_out_iter end(end_str, end_str);
		std::string u8(iter, end);

		php_write((void*)u8.c_str(), u8.size());
#else
		std::stringstream ss;
		std::string str;

		parle::lexer::debug::dump(lex.sm, lex.rules, ss);
		str = ss.str();
		php_write((void*)str.c_str(), str.size());
#endif
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}
 /* }}} */
 
/* {{{ public void Lexer::dump(void) */
PHP_METHOD(ParleLexer, dump)
{
	_lexer_dump<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::dump(void) */
PHP_METHOD(ParleRLexer, dump)
{
	_lexer_dump<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
}
/* }}} */

template <typename parser_obj_type> bool
_parser_is_in_reduce_state(parser_obj_type& par, bool php_throw = true)
{/*{{{*/
	bool ret = par.results.entry.action == parsertl::action::reduce;

	if (!ret && php_throw)
		zend_throw_exception_ex(ParleParserException_ce, 0, "Not in a reduce state!");

	return ret;
}
/* }}} */

template <typename parser_obj_type> void
_parser_token(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *tok;

	/* XXX map the full signature. */
	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &tok) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.token(PARLE_CVT_U32(ZSTR_VAL(tok)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::token(string $token) */
PHP_METHOD(ParleParser, token)
{
	_parser_token<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::token(string $token) */
PHP_METHOD(ParleRParser, token)
{
	_parser_token<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_left(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &tok) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.left(PARLE_CVT_U32(ZSTR_VAL(tok)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::left(string $token) */
PHP_METHOD(ParleParser, left)
{
	_parser_left<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::left(string $token) */
PHP_METHOD(ParleRParser, left)
{
	_parser_left<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_right(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &tok) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));


	try {
		auto &rules = zppo->par->rules;
		rules.right(PARLE_CVT_U32(ZSTR_VAL(tok)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::right(string $token) */
PHP_METHOD(ParleParser, right)
{
	_parser_right<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::right(string $token) */
PHP_METHOD(ParleRParser, right)
{
	_parser_right<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_precedence(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &tok) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.precedence(PARLE_CVT_U32(ZSTR_VAL(tok)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::precedence(string $token) */
PHP_METHOD(ParleParser, precedence)
{
	_parser_precedence<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::precedence(string $token) */
PHP_METHOD(ParleRParser, precedence)
{
	_parser_precedence<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_nonassoc(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &tok) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.nonassoc(PARLE_CVT_U32(ZSTR_VAL(tok)));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::nonassoc(string $token) */
PHP_METHOD(ParleParser, nonassoc)
{
	_parser_nonassoc<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::nonassoc(string $token) */
PHP_METHOD(ParleRParser, nonassoc)
{
	_parser_nonassoc<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_build(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
		parle::parser::generator::build(par.rules, par.sm);
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::build(void) */
PHP_METHOD(ParleParser, build)
{
	_parser_build<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::build(void) */
PHP_METHOD(ParleRParser, build)
{
	_parser_build<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_push(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *lhs, *rhs;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSS", &me, ce, &lhs, &rhs) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		RETURN_LONG(static_cast<zend_long>(rules.push(PARLE_CVT_U32(ZSTR_VAL(lhs)), PARLE_CVT_U32(ZSTR_VAL(rhs)))));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public int Parser::push(string $name, string $rule) */
PHP_METHOD(ParleParser, push)
{
	_parser_push<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public int RParser::push(string $name, string $rule) */
PHP_METHOD(ParleRParser, push)
{
	_parser_push<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type, typename lexer_obj_type> void
_parser_validate(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *par_ce, zend_class_entry *lex_ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	lexer_obj_type *zplo;
	zval *me, *zlex;
	zend_string *in;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSO", &me, par_ce, &in, &zlex, lex_ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));
	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(zlex));

	try {
		auto &par = *zppo->par;
		par.lex = zplo->lex;
		auto &lex = *par.lex;
		if (lex.sm.empty()) {
			zend_throw_exception(ParleLexerException_ce, "Lexer state machine is empty", 0);
			return;
		} else if (par.sm.empty()) {
			zend_throw_exception(ParleParserException_ce, "Parser state machine is empty", 0);
			return;
		}
		lex.in = PARLE_SCVT_U32(ZSTR_VAL(in));
		lex.iter = {lex.in.begin(), lex.in.end(), lex, true};
		lex.par = zppo->par;
		par.productions = {};
		par.results = {lex.iter->id, par.sm};
		RETURN_BOOL(parsertl::parse(lex.iter, par.sm, par.results));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}

	RETURN_FALSE;
}/*}}}*/

/* {{{ public boolean Parser::validate(void) */
PHP_METHOD(ParleParser, validate)
{
	_parser_validate<ze_parle_parser_obj, ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce, ParleLexer_ce);
}
/* }}} */

/* {{{ public boolean RParser::validate(void) */
PHP_METHOD(ParleRParser, validate)
{
	_parser_validate<ze_parle_rparser_obj, ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce, ParleRLexer_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_tokenId(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *nom;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &nom) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		RETURN_LONG(rules.token_id(PARLE_CVT_U32(ZSTR_VAL(nom))));
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public int Parser::tokenId(string $tok) */
PHP_METHOD(ParleParser, tokenId)
{
	_parser_tokenId<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public int RParser::tokenId(string $tok) */
PHP_METHOD(ParleRParser, tokenId)
{
	_parser_tokenId<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_sigil(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_long idx = 0;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l", &me, ce, &idx) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	auto &par = *zppo->par;

	if (!_parser_is_in_reduce_state(par)) {
		return;
	} else if (idx < Z_L(0) ||
		par.productions.size() - par.results.production_size(par.sm, par.results.entry.param) + static_cast<size_t>(idx) >= par.productions.size()) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Invalid index " ZEND_LONG_FMT, idx);
		return;
	}

	try {
		auto &lex = *par.lex;
		auto ret = par.results.dollar(static_cast<parle::id_type>(idx), par.sm, par.productions);
		size_t start_pos = ret.first - lex.in.begin();
		parle::string r(lex.in, start_pos, ret.second - ret.first);
		std::string r8 = PARLE_SCVT_U8(r);
		RETURN_STRINGL(r8.c_str(), r8.size());
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public string Parser::sigil(int $idx) */
PHP_METHOD(ParleParser, sigil)
{
	_parser_sigil<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public string RParser::sigil(int $idx) */
PHP_METHOD(ParleRParser, sigil)
{
	_parser_sigil<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_sigil_name(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	try {
		parser_obj_type *zppo;
		zval *me;
		zend_long idx = 0;

		if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l", &me, ce, &idx) == FAILURE) {
			return;
		}

		zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

		auto &par = *zppo->par;

		if (!_parser_is_in_reduce_state(par)) {
			return;
		} else if (idx < Z_L(0) ||
			par.productions.size() - par.results.production_size(par.sm, par.results.entry.param) + static_cast<size_t>(idx) >= par.productions.size()) {
			zend_throw_exception_ex(ParleParserException_ce, 0, "Invalid index " ZEND_LONG_FMT, idx);
			return;
		}

		std::size_t id = par.sm._rules.
			at(par.results.entry.param).second[idx];
		bool token = id < par.rules.terminals_count();
		parle::string name;
		std::string r8;

		if (token)
		{
			name = par.rules.name_from_token_id(id);
		}
		else
		{
			id -= par.rules.terminals_count();
			name = par.rules.name_from_nt_id(id);
		}

		r8 = PARLE_SCVT_U8(name);
		RETURN_STRINGL(r8.c_str(), r8.size());
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public Parser\SigilName Parser::sigilName(int $idx) */
PHP_METHOD(ParleParser, sigilName)
{
	_parser_sigil_name<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public Parser\SigilName RParser::sigilName(int $idx) */
PHP_METHOD(ParleRParser, sigilName)
{
	_parser_sigil_name<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_advance(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
		auto &lex = *par.lex;
		if (nullptr == par.lex) {
			zend_throw_exception(ParleLexerException_ce, "No Lexer supplied", 0);
			return;
		} else if (lex.sm.empty()) {
			zend_throw_exception(ParleLexerException_ce, "Lexer state machine is empty", 0);
			return;
		} else if (par.sm.empty()) {
			zend_throw_exception(ParleParserException_ce, "Parser state machine is empty", 0);
			return;
		}
		parsertl::lookup(lex.iter, par.sm, par.results, par.productions);
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::advance(void) */
PHP_METHOD(ParleParser, advance)
{
	_parser_advance<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::advance(void) */
PHP_METHOD(ParleRParser, advance)
{
	_parser_advance<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type, typename lexer_obj_type> void
_parser_consume(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *par_ce, zend_class_entry *lex_ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	lexer_obj_type *zplo;
	zval *me, *zlex;
	zend_string *in;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSO", &me, par_ce, &in, &zlex, lex_ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));
	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(zlex));

	try {
		auto &par = *zppo->par;
		par.lex = zplo->lex;
		auto &lex = *par.lex;
		if (lex.sm.empty()) {
			zend_throw_exception(ParleLexerException_ce, "Lexer state machine is empty", 0);
			return;
		} else if (par.sm.empty()) {
			zend_throw_exception(ParleParserException_ce, "Parser state machine is empty", 0);
			return;
		}
		lex.in = PARLE_CVT_U32(ZSTR_VAL(in));
		lex.iter = {lex.in.begin(), lex.in.end(), lex, true};
		lex.par = zppo->par;
		par.productions = {};
		par.results = {lex.iter->id, par.sm};
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::consume(string $s, Lexer $lex) */
PHP_METHOD(ParleParser, consume)
{
	_parser_consume<ze_parle_parser_obj, ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RParser::consume(string $s, RLexer $lex) */
PHP_METHOD(ParleRParser, consume)
{
	_parser_consume<ze_parle_rparser_obj, ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce, ParleRLexer_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_dump(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
#if PARLE_U32
		std::basic_stringstream<parle::char_type> ss;
		parle::string str;

		parle::parser::debug::dump(par.rules, ss);
		str = ss.str();

		const parle::char_type* end_str = str.c_str() + str.size();
		parle::utf_out_iter iter(str.c_str(), end_str);
		parle::utf_out_iter end(end_str, end_str);
		std::string u8(iter, end);

		php_write((void*)u8.c_str(), u8.size());
#else
		std::stringstream ss;
		std::string str;

		parsertl::debug::dump(par.rules, ss);
		str = ss.str();
		php_write((void*)str.c_str(), str.size());
#endif
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::dump(void) */
PHP_METHOD(ParleParser, dump)
{
	_parser_dump<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::dump(void) */
PHP_METHOD(ParleRParser, dump)
{
	_parser_dump<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_trace(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));
	auto &par = *zppo->par;

	try {
		//std::string s;
		parle::string s;
		std::string s8;
		auto &results = par.results;
		auto &entry = results.entry;
		switch (entry.action) {
			case parsertl::action::shift:
				s = PARLE_PRE_U32("shift ") + PARLE_SCVT_U32(std::to_string(entry.param));
				s8 = PARLE_SCVT_U8(s);
				RETURN_STRINGL(s8.c_str(), s8.size());
				break;
			case parsertl::action::go_to:
				s = PARLE_PRE_U32("goto ") + PARLE_SCVT_U32(std::to_string(entry.param));
				s8 = PARLE_SCVT_U8(s);
				RETURN_STRINGL(s8.c_str(), s8.size());
				break;
			case parsertl::action::accept:
				RETURN_STRINGL("accept", sizeof("accept")-1);
				break;
			case parsertl::action::reduce: {
				/* TODO symbols should be setup only once. */
				//parsertl::rules::string_vector symbols;
				std::vector<parle::string> symbols;
				par.rules.terminals(symbols);
				par.rules.non_terminals(symbols);
				parle::parser::state_machine::id_type_vector_pair &pair_ = par.sm._rules[entry.param];

				s = PARLE_PRE_U32("reduce by ") + symbols[pair_.first] + PARLE_PRE_U32(" ->");

				if (pair_.second.empty()) {
					s += PARLE_PRE_U32(" %empty");
				} else {
					for (auto iter_ = pair_.second.cbegin(), end_ = pair_.second.cend(); iter_ != end_; ++iter_) {
						s += PARLE_PRE_U32(' ');
						s += symbols[*iter_];
					}
				}

				s8 = PARLE_SCVT_U8(s);
				RETURN_STRINGL(s8.c_str(), s8.size());
				}
				break;
			case parsertl::action::error:
				// pass
				break;
		}
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public string Parser::trace(void) */
PHP_METHOD(ParleParser, trace)
{
	_parser_trace<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public string RParser::trace(void) */
PHP_METHOD(ParleRParser, trace)
{
	_parser_trace<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_errorinfo(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	auto &par = *zppo->par;
	auto &lex = *par.lex;

	object_init_ex(return_value, ParleErrorInfo_ce);

	if (par.results.entry.action != parsertl::action::error) {
		return;
	} else if (nullptr == par.lex) {
		zend_throw_exception(ParleParserException_ce, "No lexer supplied", 0);
		return;
	}

	try {
		add_property_long_ex(return_value, "id", sizeof("id")-1, static_cast<zend_long>(par.results.entry.param));
		add_property_long_ex(return_value, "position", sizeof("position")-1, static_cast<zend_long>(lex.iter->first - lex.in.begin()));
		zval token;
		std::string ret = PARLE_SCVT_U8(lex.iter->str());
		object_init_ex(&token, ParleToken_ce);
		add_property_long_ex(&token, "id", sizeof("id")-1, lex.iter->id);
#if PHP_MAJOR_VERSION > 7 || PHP_MAJOR_VERSION >= 7 && PHP_MINOR_VERSION >= 2
		add_property_stringl_ex(&token, "value", sizeof("value")-1, ret.c_str(), ret.size());
#else
		add_property_stringl_ex(&token, "value", sizeof("value")-1, (char *)ret.c_str(), ret.size());
#endif
		add_property_zval_ex(return_value, "token", sizeof("token")-1, &token);
		/* TODO provide details also for other error types, if possible. */
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public Parle\ErrorInfo Parser::errorInfo(void) */
PHP_METHOD(ParleParser, errorInfo)
{
	_parser_errorinfo<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public Parle\ErrorInfo RParser::errorInfo(void) */
PHP_METHOD(ParleRParser, errorInfo)
{
	_parser_errorinfo<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_reset(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_long tid;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol", &me, ce, &tid) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));
	auto &par = *zppo->par;

	par.results.reset(static_cast<parle::id_type>(tid), par.sm);
}
/* }}} */

template <typename parser_obj_type> void
_parser_sigil_count(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	parser_obj_type *zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));
	auto& par = *zppo->par;

	if (!_parser_is_in_reduce_state(par)) {
		return;
	}

	try
	{
		RETURN_LONG(par.results.production_size(par.sm,
			par.results.entry.param));
	}
	catch (const std::exception &e)
	{
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}

	return;
}
/* }}} */

/* {{{ public void Parser::reset(int $tokenId) */
PHP_METHOD(ParleParser, reset)
{
	_parser_reset<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::reset(int $tokenId) */
PHP_METHOD(ParleRParser, reset)
{
	_parser_reset<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

/* {{{ public int Parser::sigilCount() */
PHP_METHOD(ParleParser, sigilCount)
{
	_parser_sigil_count<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public int RParser::sigilCount() */
PHP_METHOD(ParleRParser, sigilCount)
{
	_parser_sigil_count<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}
/* }}} */

template <typename parser_obj_type> void
_parser_read_bison(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *me;
	zend_string *input;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ce, &input) == FAILURE) {
		return;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		parle::string bison(PARLE_CVT_U32(ZSTR_VAL(input)));

		rules.clear();
		parsertl::read_bison(bison.c_str(),
			bison.c_str() + bison.size(),
			rules);
	} catch (const std::exception &e) {
		php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Parser::readBison(string $input) */
PHP_METHOD(ParleParser, readBison)
{
	_parser_read_bison<ze_parle_parser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleParser_ce);
}
/* }}} */

/* {{{ public void RParser::readBison(string $input) */
PHP_METHOD(ParleRParser, readBison)
{
	_parser_read_bison<ze_parle_rparser_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRParser_ce);
}

/* {{{ public void Stack::pop(void) */
PHP_METHOD(ParleStack, pop)
{
	ze_parle_stack_obj *zpso;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleStack_ce) == FAILURE) {
		return;
	}

	zpso = php_parle_stack_fetch_obj(Z_OBJ_P(me));

	if (zpso->stack->empty()) {
		return;
	}

	zval *z = zpso->stack->top();

	zpso->stack->pop();

	zval_ptr_dtor(z);
	efree(z);
}
/* }}} */

/* {{{ public void Stack::push(mixed $val) */
PHP_METHOD(ParleStack, push)
{
	ze_parle_stack_obj *zpso;
	zval *me;
	zval *in, *save;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oz", &me, ParleStack_ce, &in) == FAILURE) {
		return;
	}

	zpso = php_parle_stack_fetch_obj(Z_OBJ_P(me));

	save = (zval *) emalloc(sizeof(zval));
	ZVAL_COPY(save, in);

	zpso->stack->push(save);
}
/* }}} */

/* {{{ Arginfo
 */

#if PHP_VERSION_ID >= 70200

#define PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX
#define PARLE_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX  ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX

#else

#define PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, NULL, allow_null)
#define PARLE_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, classname, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, IS_OBJECT, #classname, allow_null)

#endif

PARLE_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_parle_lexer_gettoken, 0, 0, Parle\\Token, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_build, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_push, 0, 0, 0)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_insertmacro, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, reg, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_consume, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_advance, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_reset, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, pos, IS_LONG, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_dump, 0, 0, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_lexer_pushstate, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, state, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_callout, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, id, IS_LONG, 0)
	ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_token, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_left, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_right, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_nonassoc, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_precedence, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_read_bison, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, input, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_build, 0, 0, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_push, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, rule, IS_STRING, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_validate, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, lexer, Parle\\Lexer, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_rparser_validate, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, lexer, Parle\\RLexer, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_tokenid, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_sigil, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, idx, IS_LONG, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_sigil_name, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, idx, IS_LONG, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_advance, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_consume, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, lexer, Parle\\Lexer, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_rparser_consume, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, lexer, Parle\\RLexer, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_reset, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, tok, IS_LONG, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_dump, 0, 0, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_trace, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_parle_parser_errorinfo, 0, 0, Parle\\ErrorInfo, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_stack_empty, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_stack_pop, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_stack_push, 0, 0, 1)
	ZEND_ARG_INFO(0, item)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_stack_size, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_sigil_count, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO();

#undef PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX
/* }}} */

/* {{{ Method and function entries
 */
const zend_function_entry parle_functions[] = {
	PHP_FE_END	/* Must be the last line in parle_functions[] */
};

const zend_function_entry ParleErrorInfo_methods[] = {
	PHP_FE_END
};

const zend_function_entry ParleToken_methods[] = {
	PHP_FE_END
};

const zend_function_entry ParleLexer_methods[] = {
	PHP_ME(ParleLexer, push, arginfo_parle_lexer_push, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, getToken, arginfo_parle_lexer_gettoken, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, build, arginfo_parle_lexer_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, consume, arginfo_parle_lexer_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, advance, arginfo_parle_lexer_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, reset, arginfo_parle_lexer_reset, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, insertMacro, arginfo_parle_lexer_insertmacro, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, dump, arginfo_parle_lexer_dump, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, callout, arginfo_parle_lexer_callout, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleRLexer_methods[] = {
	PHP_ME(ParleRLexer, push, arginfo_parle_lexer_push, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, getToken, arginfo_parle_lexer_gettoken, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, build, arginfo_parle_lexer_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, consume, arginfo_parle_lexer_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, advance, arginfo_parle_lexer_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, reset, arginfo_parle_lexer_reset, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, pushState, arginfo_parle_lexer_pushstate, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, insertMacro, arginfo_parle_lexer_insertmacro, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, dump, arginfo_parle_lexer_dump, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, callout, arginfo_parle_lexer_callout, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleParser_methods[] = {
	PHP_ME(ParleParser, token, arginfo_parle_parser_token, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, left, arginfo_parle_parser_left, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, right, arginfo_parle_parser_right, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, nonassoc, arginfo_parle_parser_nonassoc, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, precedence, arginfo_parle_parser_precedence, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, build, arginfo_parle_parser_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, push, arginfo_parle_parser_push, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, validate, arginfo_parle_parser_validate, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, tokenId, arginfo_parle_parser_tokenid, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, sigil, arginfo_parle_parser_sigil, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, sigilName, arginfo_parle_parser_sigil_name, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, advance, arginfo_parle_parser_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, consume, arginfo_parle_parser_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, dump, arginfo_parle_parser_dump, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, trace, arginfo_parle_parser_trace, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, errorInfo, arginfo_parle_parser_errorinfo, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, reset, arginfo_parle_parser_reset, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, sigilCount, arginfo_parle_parser_sigil_count, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, readBison, arginfo_parle_parser_read_bison, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleRParser_methods[] = {
	PHP_ME(ParleRParser, token, arginfo_parle_parser_token, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, left, arginfo_parle_parser_left, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, right, arginfo_parle_parser_right, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, nonassoc, arginfo_parle_parser_nonassoc, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, precedence, arginfo_parle_parser_precedence, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, build, arginfo_parle_parser_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, push, arginfo_parle_parser_push, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, validate, arginfo_parle_rparser_validate, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, tokenId, arginfo_parle_parser_tokenid, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, sigil, arginfo_parle_parser_sigil, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, sigilName, arginfo_parle_parser_sigil_name, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, advance, arginfo_parle_parser_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, consume, arginfo_parle_rparser_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, dump, arginfo_parle_parser_dump, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, trace, arginfo_parle_parser_trace, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, errorInfo, arginfo_parle_parser_errorinfo, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, reset, arginfo_parle_parser_reset, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, sigilCount, arginfo_parle_parser_sigil_count, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRParser, readBison, arginfo_parle_parser_read_bison, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleStack_methods[] = {
	PHP_ME(ParleStack, pop, arginfo_parle_stack_pop, ZEND_ACC_PUBLIC)
	PHP_ME(ParleStack, push, arginfo_parle_stack_push, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */

/* {{{ Prop handler macros */
#if PHP_VERSION_ID < 80000

#define PARLE_IS_PROP(name) (zend_binary_strcmp(name, sizeof(name) - 1, Z_STRVAL_P(member), Z_STRLEN_P(member)) == 0)
#define PARLE_CHECK_THROW_RO_PROP_EX(ex_ce, prop, action) \
	if (PARLE_IS_PROP(prop)) { \
		zend_throw_exception_ex(ex_ce, 0, "Cannot set readonly property $%s of class %s", prop, ZSTR_VAL(Z_OBJ_P(object)->ce->name)); \
		if (member == &tmp_member) { \
			zval_dtor(member); \
		} \
		action; \
	}
#else

#define PARLE_IS_PROP(name) (zend_binary_strcmp(name, sizeof(name) - 1, ZSTR_VAL(member), ZSTR_LEN(member)) == 0)
#define PARLE_CHECK_THROW_RO_PROP_EX(ex_ce, prop, action) \
	if (PARLE_IS_PROP(prop)) { \
		zend_throw_exception_ex(ex_ce, 0, "Cannot set readonly property $%s of class %s", prop, ZSTR_VAL(object->ce->name)); \
		action; \
	}

#endif

#define PARLE_LEX_CHECK_THROW_RET_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleLexerException_ce, prop, return &EG(uninitialized_zval))
#define PARLE_LEX_CHECK_THROW_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleLexerException_ce, prop, return)
#define PARLE_PAR_CHECK_THROW_RET_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleParserException_ce, prop, return &EG(uninitialized_zval))
#define PARLE_PAR_CHECK_THROW_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleParserException_ce, prop, return)
#define PARLE_STACK_CHECK_THROW_RET_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleStackException_ce, prop, return &EG(uninitialized_zval))
#define PARLE_STACK_CHECK_THROW_RO_PROP(prop) PARLE_CHECK_THROW_RO_PROP_EX(ParleStackException_ce, prop, return)
/* }}} */

template<typename lexer_type> void
php_parle_lexer_obj_dtor(lexer_type *zplo) noexcept
{/*{{{*/
	zend_object_std_dtor(&zplo->zo);

	auto it = zplo->lex->cb_map.begin();
	while (zplo->lex->cb_map.end() != it) {
		zval_ptr_dtor(&it->second.cb);
		zplo->lex->cb_map.erase(it++);
	}

	delete zplo->lex;
}/*}}}*/

static void
php_parle_lexer_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_lexer_obj *zplo = php_parle_lexer_fetch_obj(obj);
	php_parle_lexer_obj_dtor<ze_parle_lexer_obj>(zplo);
}/*}}}*/

static void
php_parle_rlexer_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_rlexer_obj *zplo = php_parle_rlexer_fetch_obj(obj);
	php_parle_lexer_obj_dtor<ze_parle_rlexer_obj>(zplo);
}/*}}}*/

template<typename lexer_obj_type> zend_object *
php_parle_lexer_obj_ctor(zend_class_entry *ce, zend_object_handlers *obj_handlers) noexcept
{/*{{{*/
	lexer_obj_type *zplo;

	typedef std::remove_pointer_t<decltype(zplo->lex)> lex_type;
//	std::cout << typeid(lex_type).name() << std::endl;
	zplo = static_cast<lexer_obj_type *>(ecalloc(1, sizeof(lex_type) + zend_object_properties_size(ce)));

	zend_object_std_init(&zplo->zo, ce);
	object_properties_init(&zplo->zo, ce);
	zplo->zo.handlers = obj_handlers;

	zplo->lex = new lex_type{};
	zplo->lex->rules.flags(*lexertl::regex_flags::dot_not_newline |
		*lexertl::regex_flags::dot_not_cr_lf);

	return &zplo->zo;
}/*}}}*/

static zend_object *
php_parle_rlexer_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_lexer_obj_ctor<ze_parle_rlexer_obj>(ce, &parle_rlexer_handlers);
}/*}}}*/

static zend_object *
php_parle_lexer_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_lexer_obj_ctor<ze_parle_lexer_obj>(ce, &parle_lexer_handlers);
}/*}}}*/

template <typename lexer_obj_type> zval * 
#if PHP_VERSION_ID < 80000
php_parle_lex_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval tmp_member;
	zval *retval = NULL;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(object));
#else
php_parle_lex_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *retval = NULL;

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(object);
#endif

	if (type != BP_VAR_R && type != BP_VAR_IS) {
		PARLE_LEX_CHECK_THROW_RET_RO_PROP("state")
		PARLE_LEX_CHECK_THROW_RET_RO_PROP("marker")
		PARLE_LEX_CHECK_THROW_RET_RO_PROP("cursor")
		PARLE_LEX_CHECK_THROW_RET_RO_PROP("line")
		PARLE_LEX_CHECK_THROW_RET_RO_PROP("column")
	}

	auto &lex = *zplo->lex;
	retval = rv;
	if (PARLE_IS_PROP("bol")) {
		ZVAL_BOOL(retval, lex.iter->bol);
	} else if (PARLE_IS_PROP("flags")) {
		ZVAL_LONG(retval, lex.rules.flags());
	} else if (PARLE_IS_PROP("state")) {
		ZVAL_LONG(retval, lex.iter->state);
	} else if (PARLE_IS_PROP("marker")) {
		ZVAL_LONG(retval, lex.iter->first - lex.in.begin());
	} else if (PARLE_IS_PROP("cursor")) {
		ZVAL_LONG(retval, lex.iter->second - lex.in.begin());
	} else if (PARLE_IS_PROP("line")) {
		ZVAL_LONG(retval, lex.iter.line);
	} else if (PARLE_IS_PROP("column")) {
		ZVAL_LONG(retval, lex.iter.column);
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

static zval * 
#if PHP_VERSION_ID < 80000
php_parle_lexer_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
#else
php_parle_lexer_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
#endif
{/*{{{*/
	return php_parle_lex_read_property<ze_parle_lexer_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

static zval * 
#if PHP_VERSION_ID < 80000
php_parle_rlexer_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
#else
php_parle_rlexer_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
#endif
{/*{{{*/
	return php_parle_lex_read_property<ze_parle_rlexer_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

template <typename lexer_obj_type>
#if PHP_VERSION_ID >= 70400
zval *
#else
void
#endif
#if PHP_VERSION_ID < 80000
php_parle_lex_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval tmp_member;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(object));
#else
php_parle_lex_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	lexer_obj_type *zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(object);
#endif

	auto &lex = *zplo->lex;
	if (PARLE_IS_PROP("bol")) {
		if (lex.par) {
			/* Iterator has it const. */
#if PHP_VERSION_ID < 80000
			zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $bol of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
#else
			zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $bol of class %s", ZSTR_VAL(object->ce->name));
#endif
		} else {
			lex.iter.set_bol(static_cast<bool>(zval_is_true(value) == 1));
		}
	} else if (PARLE_IS_PROP("flags")) {
		lex.rules.flags(zval_get_long(value));
	}
#if PHP_VERSION_ID >= 70400
	  else PARLE_LEX_CHECK_THROW_RET_RO_PROP("state")
	  else PARLE_LEX_CHECK_THROW_RET_RO_PROP("cursor")
	  else PARLE_LEX_CHECK_THROW_RET_RO_PROP("marker")
	  else PARLE_LEX_CHECK_THROW_RET_RO_PROP("line")
	  else PARLE_LEX_CHECK_THROW_RET_RO_PROP("column")
#else
	  else PARLE_LEX_CHECK_THROW_RO_PROP("state")
	  else PARLE_LEX_CHECK_THROW_RO_PROP("cursor")
	  else PARLE_LEX_CHECK_THROW_RO_PROP("marker")
	  else PARLE_LEX_CHECK_THROW_RO_PROP("line")
	  else PARLE_LEX_CHECK_THROW_RO_PROP("column")
#endif
	else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

#if PHP_VERSION_ID >= 70400
	return value;
#endif
}/*}}}*/

#if PHP_VERSION_ID >= 70400
static zval *
#else
static void
#endif
#if PHP_VERSION_ID < 80000
php_parle_lexer_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
#else
php_parle_lexer_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
#endif
{/*{{{*/
#if PHP_VERSION_ID >= 70400
	return
#endif
	php_parle_lex_write_property<ze_parle_lexer_obj>(object, member, value, cache_slot);
}/*}}}*/

#if PHP_VERSION_ID >= 70400
static zval *
#else
static void
#endif
#if PHP_VERSION_ID < 80000
php_parle_rlexer_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
#else
php_parle_rlexer_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
#endif
{/*{{{*/
#if PHP_VERSION_ID >= 70400
	return
#endif
	php_parle_lex_write_property<ze_parle_rlexer_obj>(object, member, value, cache_slot);
}/*}}}*/

template <typename lexer_obj_type> HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_lex_get_properties(zval *object) noexcept
{/*{{{*/
	lexer_obj_type *zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(object));
#else
php_parle_lex_get_properties(zend_object *object) noexcept
{/*{{{*/
	lexer_obj_type *zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(object);
#endif
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);

	auto &lex = *zplo->lex;
	ZVAL_LONG(&zv, lex.rules.flags());
	zend_hash_str_update(props, "flags", sizeof("flags")-1, &zv);
	ZVAL_BOOL(&zv, lex.iter->bol);
	zend_hash_str_update(props, "bol", sizeof("bol")-1, &zv);
	ZVAL_LONG(&zv, lex.iter->state);
	zend_hash_str_update(props, "state", sizeof("state")-1, &zv);
	ZVAL_LONG(&zv, lex.iter->first - lex.in.begin());
	zend_hash_str_update(props, "marker", sizeof("marker")-1, &zv);
	ZVAL_LONG(&zv, lex.iter->second - lex.in.begin());
	zend_hash_str_update(props, "cursor", sizeof("cursor")-1, &zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(lex.iter.line));
	zend_hash_str_update(props, "line", sizeof("line")-1, &zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(lex.iter.column));
	zend_hash_str_update(props, "coulmn", sizeof("column")-1, &zv);

	return props;
}/*}}}*/

static HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_lexer_get_properties(zval *object) noexcept
#else
php_parle_lexer_get_properties(zend_object *object) noexcept
#endif
{/*{{{*/
	return php_parle_lex_get_properties<ze_parle_lexer_obj>(object);
}/*}}}*/

static HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_rlexer_get_properties(zval *object) noexcept
#else
php_parle_rlexer_get_properties(zend_object *object) noexcept
#endif
{/*{{{*/
	return php_parle_lex_get_properties<ze_parle_rlexer_obj>(object);
}/*}}}*/

template <typename lexer_obj_type> static int
#if PHP_VERSION_ID < 80000
php_parle_lex_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, rv, *prop;
	int retval = 0;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_lex_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval rv, *prop;
	int retval = 0;
#endif

    prop = php_parle_lex_read_property<lexer_obj_type>(object, member, BP_VAR_IS, cache_slot, &rv);

	if (prop != &EG(uninitialized_zval)) {
		if (type == 2) {
			retval = 1;
		} else if (type == 1) {
			retval = zend_is_true(prop);
		} else if (type == 0) {
			retval = (Z_TYPE_P(prop) != IS_NULL);
		}
	} else {
		retval = (zend_get_std_object_handlers())->has_property(object, member, type, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

static int
#if PHP_VERSION_ID < 80000
php_parle_lexer_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_lexer_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_lex_has_property<ze_parle_lexer_obj>(object, member, type, cache_slot);
}/*}}}*/

static int
#if PHP_VERSION_ID < 80000
php_parle_rlexer_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_rlexer_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_lex_has_property<ze_parle_rlexer_obj>(object, member, type, cache_slot);
}/*}}}*/

static HashTable *
#if PHP_VERSION_ID < 80000
php_parle_lex_get_gc(zval *object, zval **gc_data, int *gc_data_count) noexcept
#else
php_parle_lex_get_gc(zend_object *object, zval **gc_data, int *gc_data_count) noexcept
#endif
{/*{{{*/
	*gc_data = NULL;
	*gc_data_count = 0;
	return zend_std_get_properties(object);
}/*}}}*/

template <typename lexer_obj_type> static zval *
#if PHP_VERSION_ID < 80000
php_parle_lex_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, *prop;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_lex_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval *prop;
#endif

	if (PARLE_IS_PROP("state") || PARLE_IS_PROP("marker") || PARLE_IS_PROP("cursor") || PARLE_IS_PROP("bol") || PARLE_IS_PROP("flags") || PARLE_IS_PROP("line") || PARLE_IS_PROP("column")) {
		/* Fallback to read_property. */
		return NULL;
	}

	prop = (zend_get_std_object_handlers())->get_property_ptr_ptr(object, member, type, cache_slot);

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif
	return prop;
}/*}}}*/

static zval *
#if PHP_VERSION_ID < 80000
php_parle_lexer_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_lexer_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_lex_get_property_ptr_ptr<ze_parle_lexer_obj>(object, member, type, cache_slot);
}/*}}}*/

static zval *
#if PHP_VERSION_ID < 80000
php_parle_rlexer_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_rlexer_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_lex_get_property_ptr_ptr<ze_parle_rlexer_obj>(object, member, type, cache_slot);
}/*}}}*/

template<typename parser_type> void
php_parle_parser_obj_dtor(parser_type *zppo) noexcept
{/*{{{*/
	zend_object_std_dtor(&zppo->zo);

	delete zppo->par;
}/*}}}*/

template<typename parser_obj_type> zend_object *
php_parle_parser_obj_ctor(zend_class_entry *ce, zend_object_handlers *obj_handlers) noexcept
{/*{{{*/
	parser_obj_type *zppo;

	typedef std::remove_pointer_t<decltype(zppo->par)> par_type;
	zppo = static_cast<parser_obj_type *>(ecalloc(1, sizeof(par_type) + zend_object_properties_size(ce)));

	zend_object_std_init(&zppo->zo, ce);
	object_properties_init(&zppo->zo, ce);
	zppo->zo.handlers = obj_handlers;

	zppo->par = new par_type{};

	return &zppo->zo;
}/*}}}*/

static void
php_parle_parser_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo = php_parle_parser_fetch_obj(obj);
	php_parle_parser_obj_dtor<ze_parle_parser_obj>(zppo);
}/*}}}*/

static void
php_parle_rparser_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_rparser_obj *zppo = php_parle_rparser_fetch_obj(obj);
	php_parle_parser_obj_dtor<ze_parle_rparser_obj>(zppo);
}/*}}}*/

static zend_object *
php_parle_parser_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_parser_obj_ctor<ze_parle_parser_obj>(ce, &parle_parser_handlers);
}/*}}}*/

static zend_object *
php_parle_rparser_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_parser_obj_ctor<ze_parle_rparser_obj>(ce, &parle_rparser_handlers);
}/*}}}*/

template<typename parser_obj_type> static zval * 
#if PHP_VERSION_ID < 80000
php_parle_par_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval tmp_member;
	zval *retval = NULL;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(object));
#else
php_parle_par_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	parser_obj_type *zppo;
	zval *retval = NULL;

	zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(object);
#endif

	if (type != BP_VAR_R && type != BP_VAR_IS) {
		PARLE_PAR_CHECK_THROW_RET_RO_PROP("action")
		PARLE_PAR_CHECK_THROW_RET_RO_PROP("reduceId")
	}

	auto &par = *zppo->par;

	retval = rv;
	if (PARLE_IS_PROP("action")) {
		ZVAL_LONG(retval, static_cast<zend_long>(par.results.entry.action));
	} else if (PARLE_IS_PROP("reduceId")) {
		try {
			ZVAL_LONG(retval, par.results.reduce_id());
		} catch (const std::exception &e) {
			php_parle_rethrow_from_cpp(ParleParserException_ce, e.what(), 0);
		}
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

static zval * 
#if PHP_VERSION_ID < 80000
php_parle_parser_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
#else
php_parle_parser_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
#endif
{/*{{{*/
	return php_parle_par_read_property<ze_parle_parser_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

static zval * 
#if PHP_VERSION_ID < 80000
php_parle_rparser_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
#else
php_parle_rparser_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
#endif
{/*{{{*/
	return php_parle_par_read_property<ze_parle_parser_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

template <typename parser_obj_type>
#if PHP_VERSION_ID >= 70400
zval *
#else
void
#endif
#if PHP_VERSION_ID < 80000
php_parle_par_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_par_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
#endif

#if PHP_VERSION_ID >= 70400
	PARLE_PAR_CHECK_THROW_RET_RO_PROP("action")
	else PARLE_PAR_CHECK_THROW_RET_RO_PROP("reduceId")
#else
	PARLE_PAR_CHECK_THROW_RO_PROP("action")
	else PARLE_PAR_CHECK_THROW_RO_PROP("reduceId")
#endif
	else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif
#if PHP_VERSION_ID >= 70400
	return value;
#endif
}/*}}}*/

#if PHP_VERSION_ID >= 70400
static zval *
#else
static void
#endif
#if PHP_VERSION_ID < 80000
php_parle_parser_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
#else
php_parle_parser_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
#endif
{/*{{{*/
#if PHP_VERSION_ID >= 70400
	return
#endif
	php_parle_par_write_property<ze_parle_parser_obj>(object, member, value, cache_slot);
}/*}}}*/

#if PHP_VERSION_ID >= 70400
static zval *
#else
static void
#endif
#if PHP_VERSION_ID < 80000
php_parle_rparser_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
#else
php_parle_rparser_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
#endif
{/*{{{*/
#if PHP_VERSION_ID >= 70400
	return
#endif
	php_parle_par_write_property<ze_parle_rparser_obj>(object, member, value, cache_slot);
}/*}}}*/

template <typename parser_obj_type> HashTable *
#if PHP_VERSION_ID < 80000
php_parle_par_get_properties(zval *object) noexcept
{/*{{{*/
	parser_obj_type *zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(Z_OBJ_P(object));
#else
php_parle_par_get_properties(zend_object *object) noexcept
{/*{{{*/
	parser_obj_type *zppo = _php_parle_parser_fetch_zobj<parser_obj_type>(object);
#endif
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);

	auto &par = *zppo->par;

	ZVAL_LONG(&zv, static_cast<zend_long>(par.results.entry.action));
	zend_hash_str_update(props, "action", sizeof("action")-1, &zv);
	if (par.results.entry.action == parsertl::action::reduce) {
		ZVAL_LONG(&zv, par.results.reduce_id());
	} else {
		ZVAL_LONG(&zv, Z_L(-1));
	}
	zend_hash_str_update(props, "reduceId", sizeof("reduceId")-1, &zv);

	return props;
}/*}}}*//*}}}*/

static HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_parser_get_properties(zval *object) noexcept
#else
php_parle_parser_get_properties(zend_object *object) noexcept
#endif
{/*{{{*/
	return php_parle_par_get_properties<ze_parle_parser_obj>(object);
}/*}}}*/

static HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_rparser_get_properties(zval *object) noexcept
#else
php_parle_rparser_get_properties(zend_object *object) noexcept
#endif
{/*{{{*/
	return php_parle_par_get_properties<ze_parle_rparser_obj>(object);
}/*}}}*/

template <typename parser_obj_type> static int
#if PHP_VERSION_ID < 80000
php_parle_par_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, rv, *prop;
	int retval = 0;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_par_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval rv, *prop;
	int retval = 0;
#endif

    prop = php_parle_par_read_property<parser_obj_type>(object, member, BP_VAR_IS, cache_slot, &rv);

	if (prop != &EG(uninitialized_zval)) {
		if (type == 2) {
			retval = 1;
		} else if (type == 1) {
			retval = zend_is_true(prop);
		} else if (type == 0) {
			retval = (Z_TYPE_P(prop) != IS_NULL);
		}
	} else {
		retval = (zend_get_std_object_handlers())->has_property(object, member, type, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

static int
#if PHP_VERSION_ID < 80000
php_parle_parser_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_parser_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_par_has_property<ze_parle_parser_obj>(object, member, type, cache_slot);
}/*}}}*/

static int
#if PHP_VERSION_ID < 80000
php_parle_rparser_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_rparser_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_par_has_property<ze_parle_rparser_obj>(object, member, type, cache_slot);
}/*}}}*/

static HashTable *
#if PHP_VERSION_ID < 80000
php_parle_par_get_gc(zval *object, zval **gc_data, int *gc_data_count) noexcept
#else
php_parle_par_get_gc(zend_object *object, zval **gc_data, int *gc_data_count) noexcept
#endif
{/*{{{*/
	*gc_data = NULL;
	*gc_data_count = 0;
	return zend_std_get_properties(object);
}/*}}}*/

template <typename parser_obj_type> static zval *
#if PHP_VERSION_ID < 80000
php_parle_par_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, *prop;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_par_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval *prop;
#endif

	if (PARLE_IS_PROP("action") || PARLE_IS_PROP("reduceId")) {
		/* Fallback to read_property. */
		return NULL;
	}

	prop = (zend_get_std_object_handlers())->get_property_ptr_ptr(object, member, type, cache_slot);

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return prop;
}/*}}}*/

static zval *
#if PHP_VERSION_ID < 80000
php_parle_parser_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_parser_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_par_get_property_ptr_ptr<ze_parle_parser_obj>(object, member, type, cache_slot);
}/*}}}*/

static zval *
#if PHP_VERSION_ID < 80000
php_parle_rparser_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
#else
php_parle_rparser_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
#endif
{/*{{{*/
	return php_parle_par_get_property_ptr_ptr<ze_parle_rparser_obj>(object, member, type, cache_slot);
}/*}}}*/

static void
php_parle_stack_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso = php_parle_stack_fetch_obj(obj);

	zend_object_std_dtor(&zpso->zo);

	if (!zpso->stack->empty()) {
		size_t sz = zpso->stack->size();
		for (size_t i = 0; i < sz; i++ ) {
			zval *z = zpso->stack->top();
			zpso->stack->pop();
			zval_ptr_dtor(z);
			efree(z);
		}
	}

	delete zpso->stack;
}/*}}}*/

static zend_object *
php_parle_stack_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;

	zpso = static_cast<ze_parle_stack_obj *>(ecalloc(1, sizeof(ze_parle_stack_obj) + zend_object_properties_size(ce)));

	zend_object_std_init(&zpso->zo, ce);
	object_properties_init(&zpso->zo, ce);
	zpso->zo.handlers = &parle_stack_handlers;

	zpso->stack = new parle::stack::stack{};

	return &zpso->zo;
}/*}}}*/

static zval * 
#if PHP_VERSION_ID < 80000
php_parle_stack_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;
	zval tmp_member;
	zval *retval = NULL;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zpso = php_parle_stack_fetch_obj(Z_OBJ_P(object));
#else
php_parle_stack_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;
	zval *retval = NULL;

	zpso = php_parle_stack_fetch_obj(object);
#endif

	if (type != BP_VAR_R && type != BP_VAR_IS) {
		PARLE_STACK_CHECK_THROW_RET_RO_PROP("empty")
		PARLE_STACK_CHECK_THROW_RET_RO_PROP("size")
	}

	retval = rv;
	if (PARLE_IS_PROP("top")) {
		if (zpso->stack->empty()) {
			ZVAL_NULL(retval);
		} else {
			ZVAL_COPY(retval, zpso->stack->top());
		}
	} else if (PARLE_IS_PROP("empty")) {
		ZVAL_BOOL(retval, zpso->stack->empty());
	} else if (PARLE_IS_PROP("size")) {
		ZVAL_LONG(retval, zpso->stack->size());
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

#if PHP_VERSION_ID >= 70400
static zval *
#else
static void
#endif
#if PHP_VERSION_ID < 80000
php_parle_stack_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;
	zval tmp_member;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zpso = php_parle_stack_fetch_obj(Z_OBJ_P(object));
#else
php_parle_stack_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;

	zpso = php_parle_stack_fetch_obj(object);
#endif

	if (PARLE_IS_PROP("top")) {
		if (zpso->stack->empty()) {
			// XXX should this be done?
			zval *z = (zval *) emalloc(sizeof(zval));
			ZVAL_COPY(z, value);

			zpso->stack->push(z);
		} else {
			zval *old = zpso->stack->top();

			zval *z = (zval *) emalloc(sizeof(zval));
			ZVAL_COPY(z, value);

			zpso->stack->top() = z;

			zval_ptr_dtor(old);
			efree(old);
		}
	}
#if PHP_VERSION_ID >= 70400
	  else PARLE_STACK_CHECK_THROW_RET_RO_PROP("empty")
	  else PARLE_STACK_CHECK_THROW_RET_RO_PROP("size")
#else
	  else PARLE_STACK_CHECK_THROW_RO_PROP("empty")
	  else PARLE_STACK_CHECK_THROW_RO_PROP("size")
#endif
	  else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif
#if PHP_VERSION_ID >= 70400
	return value;
#endif
}/*}}}*/

static HashTable * 
#if PHP_VERSION_ID < 80000
php_parle_stack_get_properties(zval *object) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso = php_parle_stack_fetch_obj(Z_OBJ_P(object));
#else
php_parle_stack_get_properties(zend_object *object) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso = php_parle_stack_fetch_obj(object);
#endif
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);

	ZVAL_BOOL(&zv, zpso->stack->empty());
	zend_hash_str_update(props, "empty", sizeof("empty")-1, &zv);
	ZVAL_LONG(&zv, zpso->stack->size());
	zend_hash_str_update(props, "size", sizeof("size")-1, &zv);
	if (zpso->stack->empty()) {
		ZVAL_NULL(&zv);
	} else {
		ZVAL_COPY(&zv, zpso->stack->top());
	}
	zend_hash_str_update(props, "top", sizeof("top")-1, &zv);
	// Not good to copy it, but otherwise need some hackish code to access the container
	array_init(&zv);
	for (parle::stack::stack tmp = *zpso->stack; !tmp.empty(); tmp.pop()) {
		zend_hash_next_index_insert(Z_ARRVAL(zv), tmp.top());
	}
	zend_hash_str_update(props, "elements", sizeof("elements")-1, &zv);

	return props;
}/*}}}*/

static int
#if PHP_VERSION_ID < 80000
php_parle_stack_has_property(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, rv, *prop;
	int retval = 0;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_stack_has_property(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval rv, *prop;
	int retval = 0;
#endif

    prop = php_parle_stack_read_property(object, member, BP_VAR_IS, cache_slot, &rv);

	if (prop != &EG(uninitialized_zval)) {
		if (type == 2) {
			retval = 1;
		} else if (type == 1) {
			retval = zend_is_true(prop);
		} else if (type == 0) {
			retval = (Z_TYPE_P(prop) != IS_NULL);
		}
	} else {
		retval = (zend_get_std_object_handlers())->has_property(object, member, type, cache_slot);
	}

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return retval;
}/*}}}*/

static HashTable *
#if PHP_VERSION_ID < 80000
php_parle_stack_get_gc(zval *object, zval **gc_data, int *gc_data_count) noexcept
#else
php_parle_stack_get_gc(zend_object *object, zval **gc_data, int *gc_data_count) noexcept
#endif
{/*{{{*/
	*gc_data = NULL;
	*gc_data_count = 0;
	return zend_std_get_properties(object);
}/*}}}*/

static zval *
#if PHP_VERSION_ID < 80000
php_parle_stack_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval tmp_member, *prop;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}
#else
php_parle_stack_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot) noexcept
{/*{{{*/
	zval *prop;
#endif

	if (PARLE_IS_PROP("top") || PARLE_IS_PROP("empty") || PARLE_IS_PROP("size")) {
		/* Fallback to read_property. */
		return NULL;
	}

	prop = (zend_get_std_object_handlers())->get_property_ptr_ptr(object, member, type, cache_slot);

#if PHP_VERSION_ID < 80000
	if (member == &tmp_member) {
		zval_dtor(member);
	}
#endif

	return prop;
}/*}}}*/

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("parle.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_parle_globals, parle_globals)
    STD_PHP_INI_ENTRY("parle.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_parle_globals, parle_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_parle_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_parle_init_globals(zend_parle_globals *parle_globals)
{
	parle_globals->global_value = 0;
	parle_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(parle)
{
	zend_class_entry ce;

	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	INIT_CLASS_ENTRY(ce, "Parle\\ErrorInfo", ParleErrorInfo_methods);
	ParleErrorInfo_ce = zend_register_internal_class(&ce);
	zend_declare_property_long(ParleErrorInfo_ce, "id", sizeof("id")-1, Z_L(0), ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleErrorInfo_ce, "position", sizeof("position")-1, Z_L(0), ZEND_ACC_PUBLIC);
	zend_declare_property_null(ParleErrorInfo_ce, "token", sizeof("token")-1, ZEND_ACC_PUBLIC);

	INIT_CLASS_ENTRY(ce, "Parle\\Token", ParleToken_methods);
	ParleToken_ce = zend_register_internal_class(&ce);

#define DECL_CONST(name, val) zend_declare_class_constant_long(ParleToken_ce, name, sizeof(name) - 1, val);
	DECL_CONST("EOI", Z_L(0))
	DECL_CONST("UNKNOWN", static_cast<zend_long>(parle::lexer::smatch::npos()));
	DECL_CONST("SKIP", static_cast<zend_long>(parle::lexer::smatch::skip()));
#undef DECL_CONST
	zend_declare_property_long(ParleToken_ce, "id", sizeof("id")-1, static_cast<zend_long>(lexertl::smatch::npos()), ZEND_ACC_PUBLIC);
	zend_declare_property_null(ParleToken_ce, "value", sizeof("value")-1, ZEND_ACC_PUBLIC);

	auto init_lexer_consts_and_props = [](zend_class_entry *ce) {
#define DECL_CONST(name, val) zend_declare_class_constant_long(ce, name, sizeof(name) - 1, val);
		DECL_CONST("ICASE", *lexertl::regex_flags::icase)
		DECL_CONST("DOT_NOT_LF", *lexertl::regex_flags::dot_not_newline)
		DECL_CONST("DOT_NOT_CRLF", *lexertl::regex_flags::dot_not_cr_lf)
		DECL_CONST("SKIP_WS", *lexertl::regex_flags::skip_ws)
		DECL_CONST("MATCH_ZERO_LEN", *lexertl::regex_flags::match_zero_len)
#undef DECL_CONST
		zend_declare_property_bool(ce, "bol", sizeof("bol")-1, 0, ZEND_ACC_PUBLIC);
		zend_declare_property_long(ce, "flags", sizeof("flags")-1, 0, ZEND_ACC_PUBLIC);
		zend_declare_property_long(ce, "state", sizeof("state")-1, 0, ZEND_ACC_PUBLIC);
		zend_declare_property_long(ce, "marker", sizeof("marker")-1, Z_L(-1), ZEND_ACC_PUBLIC);
		zend_declare_property_long(ce, "cursor", sizeof("cursor")-1, Z_L(-1), ZEND_ACC_PUBLIC);
#if PHP_VERSION_ID < 80100
		ce->serialize = zend_class_serialize_deny;
		ce->unserialize = zend_class_unserialize_deny;
#else
		ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#endif
	};

	memcpy(&parle_lexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_lexer_handlers.clone_obj = NULL;
	parle_lexer_handlers.offset = XtOffsetOf(ze_parle_lexer_obj, zo);
	parle_lexer_handlers.free_obj = php_parle_lexer_obj_destroy;
	parle_lexer_handlers.read_property = php_parle_lexer_read_property;
	parle_lexer_handlers.write_property = php_parle_lexer_write_property;
	parle_lexer_handlers.get_properties = php_parle_lexer_get_properties;
	parle_lexer_handlers.has_property = php_parle_lexer_has_property;
	parle_lexer_handlers.get_gc = php_parle_lex_get_gc;
	parle_lexer_handlers.get_property_ptr_ptr = php_parle_lexer_get_property_ptr_ptr;
	INIT_CLASS_ENTRY(ce, "Parle\\Lexer", ParleLexer_methods);
	ce.create_object = php_parle_lexer_object_init;
	ParleLexer_ce = zend_register_internal_class(&ce);
	init_lexer_consts_and_props(ParleLexer_ce);

	memcpy(&parle_rlexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_rlexer_handlers.clone_obj = NULL;
	parle_rlexer_handlers.offset = XtOffsetOf(ze_parle_rlexer_obj, zo);
	parle_rlexer_handlers.free_obj = php_parle_rlexer_obj_destroy;
	parle_rlexer_handlers.read_property = php_parle_rlexer_read_property;
	parle_rlexer_handlers.write_property = php_parle_rlexer_write_property;
	parle_rlexer_handlers.get_properties = php_parle_rlexer_get_properties;
	parle_rlexer_handlers.has_property = php_parle_rlexer_has_property;
	parle_rlexer_handlers.get_gc = php_parle_lex_get_gc;
	parle_rlexer_handlers.get_property_ptr_ptr = php_parle_rlexer_get_property_ptr_ptr;
	INIT_CLASS_ENTRY(ce, "Parle\\RLexer", ParleRLexer_methods);
	ce.create_object = php_parle_rlexer_object_init;
	ParleRLexer_ce = zend_register_internal_class(&ce);
	init_lexer_consts_and_props(ParleRLexer_ce);

	auto init_parser_consts_and_props = [](zend_class_entry *ce) {
#define DECL_CONST(name, val) zend_declare_class_constant_long(ce, name, sizeof(name) - 1, val);
		DECL_CONST("ACTION_ERROR", (zend_long)parsertl::action::error)
		DECL_CONST("ACTION_SHIFT", (zend_long)parsertl::action::shift)
		DECL_CONST("ACTION_REDUCE", (zend_long)parsertl::action::reduce)
		DECL_CONST("ACTION_GOTO", (zend_long)parsertl::action::go_to)
		DECL_CONST("ACTION_ACCEPT", (zend_long)parsertl::action::accept)
		DECL_CONST("ERROR_SYNTAX", (zend_long)parsertl::error_type::syntax_error)
		DECL_CONST("ERROR_NON_ASSOCIATIVE", (zend_long)parsertl::error_type::non_associative)
		DECL_CONST("ERROR_UNKNOWN_TOKEN", (zend_long)parsertl::error_type::unknown_token)
#undef DECL_CONST
		zend_declare_property_long(ce, "action", sizeof("action")-1, 0, ZEND_ACC_PUBLIC);
		zend_declare_property_long(ce, "reduceId", sizeof("reduceId")-1, 0, ZEND_ACC_PUBLIC);
#if PHP_VERSION_ID < 80100
		ce->serialize = zend_class_serialize_deny;
		ce->unserialize = zend_class_unserialize_deny;
#else
		ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#endif
	};

	memcpy(&parle_parser_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_parser_handlers.clone_obj = NULL;
	parle_parser_handlers.offset = XtOffsetOf(ze_parle_parser_obj, zo);
	parle_parser_handlers.free_obj = php_parle_parser_obj_destroy;
	parle_parser_handlers.read_property = php_parle_parser_read_property;
	parle_parser_handlers.write_property = php_parle_parser_write_property;
	parle_parser_handlers.get_properties = php_parle_parser_get_properties;
	parle_parser_handlers.has_property = php_parle_parser_has_property;
	parle_parser_handlers.get_gc = php_parle_par_get_gc;
	parle_parser_handlers.get_property_ptr_ptr = php_parle_parser_get_property_ptr_ptr;
	INIT_CLASS_ENTRY(ce, "Parle\\Parser", ParleParser_methods);
	ce.create_object = php_parle_parser_object_init;
	ParleParser_ce = zend_register_internal_class(&ce);
	init_parser_consts_and_props(ParleParser_ce);

	memcpy(&parle_rparser_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_rparser_handlers.clone_obj = NULL;
	parle_rparser_handlers.offset = XtOffsetOf(ze_parle_rparser_obj, zo);
	parle_rparser_handlers.free_obj = php_parle_rparser_obj_destroy;
	parle_rparser_handlers.read_property = php_parle_rparser_read_property;
	parle_rparser_handlers.write_property = php_parle_rparser_write_property;
	parle_rparser_handlers.get_properties = php_parle_rparser_get_properties;
	parle_rparser_handlers.has_property = php_parle_rparser_has_property;
	parle_rparser_handlers.get_gc = php_parle_par_get_gc;
	parle_rparser_handlers.get_property_ptr_ptr = php_parle_rparser_get_property_ptr_ptr;
	INIT_CLASS_ENTRY(ce, "Parle\\RParser", ParleRParser_methods);
	ce.create_object = php_parle_rparser_object_init;
	ParleRParser_ce = zend_register_internal_class(&ce);
	init_parser_consts_and_props(ParleRParser_ce);

	memcpy(&parle_stack_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_stack_handlers.clone_obj = NULL;
	parle_stack_handlers.offset = XtOffsetOf(ze_parle_stack_obj, zo);
	parle_stack_handlers.free_obj = php_parle_stack_obj_destroy;
	parle_stack_handlers.read_property = php_parle_stack_read_property;
	parle_stack_handlers.write_property = php_parle_stack_write_property;
	parle_stack_handlers.get_properties = php_parle_stack_get_properties;
	parle_stack_handlers.has_property = php_parle_stack_has_property;
	parle_stack_handlers.get_gc = php_parle_stack_get_gc;
	parle_stack_handlers.get_property_ptr_ptr = php_parle_stack_get_property_ptr_ptr;
	INIT_CLASS_ENTRY(ce, "Parle\\Stack", ParleStack_methods);
	ce.create_object = php_parle_stack_object_init;
	ParleStack_ce = zend_register_internal_class(&ce);
	zend_declare_property_bool(ParleStack_ce, "empty", sizeof("empty")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleStack_ce, "size", sizeof("size")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleStack_ce, "top", sizeof("top")-1, 0, ZEND_ACC_PUBLIC);
#if PHP_VERSION_ID < 80100
	ParleStack_ce->serialize = zend_class_serialize_deny;
	ParleStack_ce->unserialize = zend_class_unserialize_deny;
#else
	ParleStack_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#endif

	INIT_CLASS_ENTRY(ce, "Parle\\LexerException", NULL);
	ParleLexerException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());
	INIT_CLASS_ENTRY(ce, "Parle\\ParserException", NULL);
	ParleParserException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());
	INIT_CLASS_ENTRY(ce, "Parle\\StackException", NULL);
	ParleStackException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());

	REGISTER_NS_BOOL_CONSTANT("Parle", "INTERNAL_UTF32", PARLE_U32, CONST_PERSISTENT | CONST_CS);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(parle)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(parle)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Lexing and parsing support", "enabled");
	php_info_print_table_row(2, "Parle version", PHP_PARLE_VERSION);
	php_info_print_table_row(2, "Parle internal UTF-32", (PARLE_U32 ? "yes" : "no"));
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ parle_module_entry
 */
zend_module_entry parle_module_entry = {
	STANDARD_MODULE_HEADER,
	"parle",
	parle_functions,
	PHP_MINIT(parle),
	PHP_MSHUTDOWN(parle),
	NULL,
	NULL,
	PHP_MINFO(parle),
	PHP_PARLE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PARLE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(parle)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
