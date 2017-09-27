/*
 * Copyright (c) 2017 Anatol Belski
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

#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
#include "lexertl/iterator.hpp"
#include "lexertl/debug.hpp"
#include "lexertl/iterator.hpp"
#include "lexertl/match_results.hpp"
#include "lexertl/state_machine.hpp"

#include "parsertl/generator.hpp"
#include "parsertl/lookup.hpp"
#include "parsertl/state_machine.hpp"
#include "parsertl/parse.hpp"
#include "parsertl/token.hpp"
#include "parsertl/debug.hpp"
#include "parsertl/rules.hpp"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "php_parle.h"

#undef lookup

namespace parle {/*{{{*/
	using id_type = uint16_t;
	using char_type = char;

	namespace parser {
		struct parser;
	}

	namespace lexer {
		using state_machine = lexertl::basic_state_machine<char_type, id_type>;
		using parle_rules = lexertl::basic_rules<char_type, char_type, id_type>;

		using cmatch = lexertl::match_results<const char_type *, id_type>;
		using crmatch = lexertl::recursive_match_results<const char_type *, id_type>;
		using citerator = lexertl::iterator<const char_type *, state_machine, cmatch>;
		using criterator = lexertl::iterator<const char_type *, state_machine, crmatch>;

		using smatch = lexertl::match_results<std::string::const_iterator, id_type>;
		using srmatch = lexertl::recursive_match_results<std::string::const_iterator, id_type>;
		using siterator = lexertl::iterator<std::string::const_iterator, state_machine, smatch>;
		using sriterator = lexertl::iterator<std::string::const_iterator, state_machine, srmatch>;

		using generator = lexertl::basic_generator<parle_rules, state_machine>;
		using debug = lexertl::basic_debug<state_machine, char_type, id_type>;

		struct lexer {
			lexer() : in(""), rules(), sm(), par(nullptr), results(), iter(), iter_prev() {}
			std::string in;
			parle_rules rules;
			state_machine sm;
			parle::parser::parser *par;
			smatch results;
			siterator iter;
			siterator iter_prev;
		};

		struct rlexer : public lexer {
			rlexer() : lexer(), results(), iter(), iter_prev() {}
			srmatch results;
			sriterator iter;
			sriterator iter_prev;
		};
	}

	namespace parser {
		using state_machine = parsertl::basic_state_machine<id_type>;
		using match_results = parsertl::basic_match_results<id_type>;
		using parle_rules = parsertl::basic_rules<char_type>;
		using generator = parsertl::basic_generator<parle_rules, id_type>;
		using parle_productions = parsertl::token<parle::lexer::sriterator>::token_vector;

		struct parser {
			parle::lexer::lexer *lex;
			parle_rules rules;
			state_machine sm;
			match_results results;
			parle_productions productions;
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

struct ze_parle_stack_obj {/*{{{*/
	parle::stack::stack *stack;
	zend_object zo;
};/*}}}*/

/* {{{ Class entries and handlers declarations. */
zend_object_handlers parle_lexer_handlers;
zend_object_handlers parle_rlexer_handlers;
zend_object_handlers parle_parser_handlers;
zend_object_handlers parle_stack_handlers;

static zend_class_entry *ParleLexer_ce;
static zend_class_entry *ParleRLexer_ce;
static zend_class_entry *ParleParser_ce;
static zend_class_entry *ParleStack_ce;
static zend_class_entry *ParleLexerException_ce;
static zend_class_entry *ParleParserException_ce;
static zend_class_entry *ParleStackException_ce;
static zend_class_entry *ParleToken_ce;
static zend_class_entry *ParleErrorInfo_ce;
/* }}} */

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

static zend_always_inline ze_parle_parser_obj *
php_parle_parser_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return (ze_parle_parser_obj *)((char *)obj - XtOffsetOf(ze_parle_parser_obj, zo));
}/*}}}*/

static zend_always_inline ze_parle_stack_obj *
php_parle_parser_stack_fetch_obj(zend_object *obj) noexcept
{/*{{{*/
	return (ze_parle_stack_obj *)((char *)obj - XtOffsetOf(ze_parle_stack_obj, zo));
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
		if (user_id < 0) user_id = lex.results.npos();
		lex.rules.push(ZSTR_VAL(regex), static_cast<parle::id_type>(id), static_cast<parle::id_type>(user_id));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void RLexer::push(...) */
PHP_METHOD(ParleRLexer, push)
{
	ze_parle_rlexer_obj *zplo;
	zend_string *regex, *dfa, *new_dfa;
	zend_long id, user_id = -1;
	zval *me;

#define PREPARE_PUSH() \
	zplo = php_parle_rlexer_fetch_obj(Z_OBJ_P(me)); \
	auto &lex = *zplo->lex; \
	if (user_id < 0) user_id = lex.results.npos();

	try {
		// Rules for INITIAL
		if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSl|l", &me, ParleRLexer_ce, &regex, &id, &user_id) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(ZSTR_VAL(regex), static_cast<parle::id_type>(id), static_cast<parle::id_type>(user_id));
		// Rules with id
		} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSlS|l", &me, ParleRLexer_ce, &dfa, &regex, &id, &new_dfa, &user_id) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(ZSTR_VAL(dfa), ZSTR_VAL(regex), static_cast<parle::id_type>(id), ZSTR_VAL(new_dfa), static_cast<parle::id_type>(user_id));
		// Rules without id
		} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSS", &me, ParleRLexer_ce, &dfa, &regex, &new_dfa) == SUCCESS) {
			PREPARE_PUSH()
			lex.rules.push(ZSTR_VAL(dfa), ZSTR_VAL(regex), ZSTR_VAL(new_dfa));
		} else {
			zend_throw_exception(ParleLexerException_ce, "Couldn't match the method signature", 0);
		}
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
#undef PREPARE_PUSH
}
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
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
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

template<typename lexer_obj_type, typename results_type> void
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
		lex.in = std::string{in};
		lex.results.reset(lex.in.begin(), lex.in.end());
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}/*}}}*/

/* {{{ public void Lexer::consume(string $s) */
PHP_METHOD(ParleLexer, consume)
{
	_lexer_consume<ze_parle_lexer_obj, parle::lexer::smatch>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::consume(string $s) */
PHP_METHOD(ParleRLexer, consume)
{
	_lexer_consume<ze_parle_rlexer_obj, parle::lexer::srmatch>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
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
		RETURN_LONG(rules.push_state(state));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

template<typename lexer_obj_type> void
_lexer_token(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ce) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	try {
		object_init_ex(return_value, ParleToken_ce);
		std::string ret = lex.results.str();
		add_property_long_ex(return_value, "id", sizeof("id")-1, static_cast<zend_long>(lex.results.id));
#if PHP_MAJOR_VERSION > 7 || PHP_MAJOR_VERSION >= 7 && PHP_MINOR_VERSION >= 2
		add_property_stringl_ex(return_value, "value", sizeof("value")-1, ret.c_str(), ret.size());
#else
		add_property_stringl_ex(return_value, "value", sizeof("value")-1, (char *)ret.c_str(), ret.size());
#endif

	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
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
		lexertl::lookup(lex.sm, lex.results);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
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
_lexer_restart(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	zval *me;
	zend_long pos;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol", &me, ce, &pos) == FAILURE) {
		return;
	}

	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(me));

	auto &lex = *zplo->lex;

	if (pos < 0 || static_cast<parle::id_type>(pos) > lex.in.length()) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "Invalid offset " ZEND_LONG_FMT, pos);
		return;
	}

	lex.results.first = lex.results.second = lex.in.begin() + pos;
}/*}}}*/

/* {{{ public void Lexer::restart(int $position) */
PHP_METHOD(ParleLexer, restart)
{
	_lexer_restart<ze_parle_lexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleLexer_ce);
}
/* }}} */

/* {{{ public void RLexer::restart(int $position) */
PHP_METHOD(ParleRLexer, restart)
{
	_lexer_restart<ze_parle_rlexer_obj>(INTERNAL_FUNCTION_PARAM_PASSTHRU, ParleRLexer_ce);
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
		lex.rules.insert_macro(ZSTR_VAL(name), ZSTR_VAL(regex));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
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
		/* XXX std::cout might be not thread safe, need to gather the right
			descriptor from the SAPI and convert to a usable stream. */
		parle::lexer::debug::dump(lex.sm, lex.rules, std::cout);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
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

/* {{{ public void Parser::token(string $token) */
PHP_METHOD(ParleParser, token)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *tok;

	/* XXX map the full signature. */
	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &tok) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.token(ZSTR_VAL(tok));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::left(string $token) */
PHP_METHOD(ParleParser, left)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &tok) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.left(ZSTR_VAL(tok));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::right(string $token) */
PHP_METHOD(ParleParser, right)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &tok) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.right(ZSTR_VAL(tok));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::precedence(string $token) */
PHP_METHOD(ParleParser, precedence)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &tok) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.precedence(ZSTR_VAL(tok));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::nonassoc(string $token) */
PHP_METHOD(ParleParser, nonassoc)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *tok;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &tok) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		rules.nonassoc(ZSTR_VAL(tok));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::build(void) */
PHP_METHOD(ParleParser, build)
{
	ze_parle_parser_obj *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleParser_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
		parle::parser::generator::build(par.rules, par.sm);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public int Parser::push(string $name, string $rule) */
PHP_METHOD(ParleParser, push)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *lhs, *rhs;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSS", &me, ParleParser_ce, &lhs, &rhs) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		RETURN_LONG(static_cast<zend_long>(rules.push(ZSTR_VAL(lhs), ZSTR_VAL(rhs))));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public boolean Parser::validate(void) */
PHP_METHOD(ParleParser, validate)
{
	ze_parle_parser_obj *zppo;
	ze_parle_lexer_obj *zplo;
	zval *me, *lex;
	zend_string *in;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSO", &me, ParleParser_ce, &in, &lex, ParleLexer_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));
	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(lex));

	try {
		auto &lex = *zplo->lex;
		auto &par = *zppo->par;

		parle::lexer::citerator iter(ZSTR_VAL(in), ZSTR_VAL(in) + ZSTR_LEN(in), lex.sm);
		
		/* Since it's not more than parse, nothing is saved into the object. */
		parle::parser::match_results results(iter->id, par.sm);

		RETURN_BOOL(parsertl::parse(par.sm, iter, results));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}

	RETURN_FALSE
}
/* }}} */

/* {{{ public int Parser::tokenId(string $tok) */
PHP_METHOD(ParleParser, tokenId)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_string *nom;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS", &me, ParleParser_ce, &nom) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &rules = zppo->par->rules;
		RETURN_LONG(rules.token_id(ZSTR_VAL(nom)));
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public string Parser::sigil(int $idx) */
PHP_METHOD(ParleParser, sigil)
{
	ze_parle_parser_obj *zppo;
	zval *me;
	zend_long idx = 0;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l", &me, ParleParser_ce, &idx) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	auto &par = *zppo->par;

	if (idx < Z_L(0) ||
		par.productions.size() - par.sm._rules[par.results.entry.param].second.size() + static_cast<size_t>(idx) >= par.productions.size()) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Invalid index " ZEND_LONG_FMT, idx);
		return;
	/*} else if (par.sm._rules[par.results.entry.param].second.empty()) {
		RETURN_NULL();*/
	}

	try {
		auto &lex = *par.lex;
		auto ret = par.results.dollar(par.sm, static_cast<parle::id_type>(idx), par.productions);
		size_t start_pos = ret.first - lex.in.begin();
		const char *in = lex.in.c_str();
		RETURN_STRINGL(in + start_pos, ret.second - ret.first);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::advance(void) */
PHP_METHOD(ParleParser, advance)
{
	ze_parle_parser_obj *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleParser_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
		auto &lex = *par.lex;
		lex.iter_prev = lex.iter;
		parsertl::lookup(par.sm, lex.iter, par.results, par.productions);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleParserException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::consume(string $s, Lexer $lex) */
PHP_METHOD(ParleParser, consume)
{
	ze_parle_parser_obj *zppo;
	ze_parle_lexer_obj *zplo;
	zval *me, *lex;
	zend_string *in;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OSO", &me, ParleParser_ce, &in, &lex, ParleLexer_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));
	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(lex));

	try {
		auto &par = *zppo->par;
		par.lex = static_cast<parle::lexer::lexer *>(zplo->lex);
		auto &lex = *par.lex;
		lex.in = ZSTR_VAL(in);
		lex.iter = parle::lexer::siterator(lex.in.begin(), lex.in.end(), lex.sm);
		lex.iter_prev = lex.iter;
		lex.par = zppo->par;
		par.productions = parle::parser::parle_productions{};
		par.results = parle::parser::match_results{lex.iter->id, par.sm};
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Parser::dump(void) */
PHP_METHOD(ParleParser, dump)
{
	ze_parle_parser_obj *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleParser_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	try {
		auto &par = *zppo->par;
		/* XXX See comment in _lexer_dump(). */
		parsertl::debug::dump(par.rules, std::cout);
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public string Parser::trace(void) */
PHP_METHOD(ParleParser, trace)
{
	ze_parle_parser_obj *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleParser_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));
	auto &par = *zppo->par;

	try {
		std::string s;
		auto &results = par.results;
		auto &entry = results.entry;
		switch (entry.action) {
			case parsertl::shift:
				s = "shift " + std::to_string(entry.param);
				RETURN_STRINGL(s.c_str(), s.size());
				break;
			case parsertl::go_to:
				s = "goto " + std::to_string(entry.param);
				RETURN_STRINGL(s.c_str(), s.size());
				break;
			case parsertl::accept:
				RETURN_STRINGL("accept", sizeof("accept")-1);
				break;
			case parsertl::reduce:
				/* TODO symbols should be setup only once. */
				parsertl::rules::string_vector symbols;
				par.rules.terminals(symbols);
				par.rules.non_terminals(symbols);
				parle::parser::state_machine::id_type_pair &pair_ = par.sm._rules[entry.param];

				s = "reduce by " + symbols[pair_.first] + " ->";

				if (pair_.second.empty()) {
					s += " %empty";
				} else {
					for (auto iter_ = pair_.second.cbegin(), end_ = pair_.second.cend(); iter_ != end_; ++iter_) {
						s += ' ';
						s += symbols[*iter_];
					}
				}

				RETURN_STRINGL(s.c_str(), s.size());
				break;
		}
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public Parle\ErrorInfo Parser::errorInfo(void) */
PHP_METHOD(ParleParser, errorInfo)
{
	ze_parle_parser_obj *zppo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleParser_ce) == FAILURE) {
		return;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(me));

	auto &par = *zppo->par;
	auto &lex = *par.lex;

	object_init_ex(return_value, ParleErrorInfo_ce);

	if (par.results.entry.action != parsertl::error) {
		return;
	}


	try {
		add_property_long_ex(return_value, "id", sizeof("id")-1, static_cast<zend_long>(par.results.entry.param));
		add_property_long_ex(return_value, "position", sizeof("position")-1, static_cast<zend_long>(lex.iter->first - lex.in.begin()));
		zval token;
		std::string ret = lex.iter->str();
		object_init_ex(&token, ParleToken_ce);
#if PHP_MAJOR_VERSION > 7 || PHP_MAJOR_VERSION >= 7 && PHP_MINOR_VERSION >= 2
		add_property_stringl_ex(&token, "value", sizeof("value")-1, ret.c_str(), ret.size());
#else
		add_property_stringl_ex(&token, "value", sizeof("value")-1, (char *)ret.c_str(), ret.size());
#endif
		add_property_zval_ex(return_value, "token", sizeof("token")-1, &token);
		/* TODO provide details also for other error types, if possible. */
	} catch (const std::exception &e) {
		zend_throw_exception(ParleLexerException_ce, e.what(), 0);
	}
}
/* }}} */

/* {{{ public void Stack::pop(void) */
PHP_METHOD(ParleStack, pop)
{
	ze_parle_stack_obj *zpso;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleStack_ce) == FAILURE) {
		return;
	}

	zpso = php_parle_parser_stack_fetch_obj(Z_OBJ_P(me));

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

	zpso = php_parle_parser_stack_fetch_obj(Z_OBJ_P(me));

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_consume, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_advance, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_restart, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, pos, IS_LONG, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_lexer_dump, 0, 0, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_lexer_pushstate, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, state, IS_STRING, 0)
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_build, 0, 0, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_push, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, rule, IS_STRING, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_validate, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_tokenid, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, tok, IS_STRING, 0)
ZEND_END_ARG_INFO();

PARLE_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_parle_parser_sigil, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, idx, IS_LONG, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_advance, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_parle_parser_consume, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_INFO(0, lexer) /* Parle\Lexer or a derivative. */
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
	PHP_ME(ParleLexer, push, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, getToken, arginfo_parle_lexer_gettoken, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, build, arginfo_parle_lexer_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, consume, arginfo_parle_lexer_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, advance, arginfo_parle_lexer_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, restart, arginfo_parle_lexer_restart, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, insertMacro, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, dump, arginfo_parle_lexer_dump, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleRLexer_methods[] = {
	PHP_ME(ParleRLexer, push, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, getToken, arginfo_parle_lexer_gettoken, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, build, arginfo_parle_lexer_build, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, consume, arginfo_parle_lexer_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, advance, arginfo_parle_lexer_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, restart, arginfo_parle_lexer_restart, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, pushState, arginfo_parle_lexer_pushstate, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, insertMacro, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleRLexer, dump, arginfo_parle_lexer_dump, ZEND_ACC_PUBLIC)
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
	PHP_ME(ParleParser, advance, arginfo_parle_parser_advance, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, consume, arginfo_parle_parser_consume, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, dump, arginfo_parle_parser_dump, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, trace, arginfo_parle_parser_trace, ZEND_ACC_PUBLIC)
	PHP_ME(ParleParser, errorInfo, arginfo_parle_parser_errorinfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

const zend_function_entry ParleStack_methods[] = {
	PHP_ME(ParleStack, pop, arginfo_parle_stack_pop, ZEND_ACC_PUBLIC)
	PHP_ME(ParleStack, push, arginfo_parle_stack_push, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */


template<typename lexer_type> void
php_parle_lexer_obj_dtor(lexer_type *zplo) noexcept
{/*{{{*/
	zend_object_std_dtor(&zplo->zo);

	delete zplo->lex;
}/*}}}*/

template<typename lexer_obj_type, typename lexer_type> zend_object *
php_parle_lexer_obj_ctor(zend_class_entry *ce) noexcept
{/*{{{*/
	lexer_obj_type *zplo;

	zplo = (lexer_obj_type *)ecalloc(1, sizeof(lexer_type) + zend_object_properties_size(ce));

	zend_object_std_init(&zplo->zo, ce);
	object_properties_init(&zplo->zo, ce);
	zplo->zo.handlers = &parle_lexer_handlers;

	zplo->lex = new lexer_type{};
//	zplo->lex->rules.flags(zplo->lex->rules.flags() | lexertl::dot_not_newline | lexertl::dot_not_cr_lf);

	return &zplo->zo;
}/*}}}*/

template <typename lexer_obj_type> zval * 
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

	auto &lex = *zplo->lex;
	bool in_parser = lex.par != nullptr;
	retval = rv;
	if (strcmp(Z_STRVAL_P(member), "bol") == 0) {
		if (in_parser) {
			ZVAL_BOOL(retval, lex.iter_prev->bol);
		} else {
			ZVAL_BOOL(retval, lex.results.bol);
		}
	} else if (strcmp(Z_STRVAL_P(member), "flags") == 0) {
		ZVAL_LONG(retval, lex.rules.flags());
	} else if (strcmp(Z_STRVAL_P(member), "state") == 0) {
		if (in_parser) {
			ZVAL_LONG(retval, lex.iter_prev->state);
		} else {
			ZVAL_LONG(retval, lex.results.state);
		}
	} else if (strcmp(Z_STRVAL_P(member), "marker") == 0) {
		if (in_parser) {
			ZVAL_LONG(retval, lex.iter_prev->first - lex.in.begin());
		} else {
			ZVAL_LONG(retval, lex.results.first - lex.in.begin());
		}
	} else if (strcmp(Z_STRVAL_P(member), "cursor") == 0) {
		if (in_parser) {
			ZVAL_LONG(retval, lex.results.second - lex.in.begin());
		} else {
			ZVAL_LONG(retval, lex.results.second - lex.in.begin());
		}
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return retval;
}/*}}}*/

template <typename lexer_obj_type> void
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

	auto &lex = *zplo->lex;
	if (strcmp(Z_STRVAL_P(member), "bol") == 0) {
		if (lex.par) {
			/* Iterator has it const. */
			zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $bol of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
		} else {
			lex.results.bol = static_cast<bool>(zval_is_true(value) == 1);
		}
	} else if (strcmp(Z_STRVAL_P(member), "flags") == 0) {
		lex.rules.flags(zval_get_long(value));
	} else if (strcmp(Z_STRVAL_P(member), "state") == 0) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $state of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else if (strcmp(Z_STRVAL_P(member), "cursor") == 0) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $cursor of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else if (strcmp(Z_STRVAL_P(member), "marker") == 0) {
		zend_throw_exception_ex(ParleLexerException_ce, 0, "Cannot set readonly property $marker of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}/*}}}*/

template <typename lexer_obj_type> HashTable * 
php_parle_lex_get_properties(zval *object) noexcept
{/*{{{*/
	lexer_obj_type *zplo;
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);
	zplo = _php_parle_lexer_fetch_zobj<lexer_obj_type>(Z_OBJ_P(object));

	auto &lex = *zplo->lex;
	if (lex.par) {
		ZVAL_LONG(&zv, lex.rules.flags());
		zend_hash_str_update(props, "flags", sizeof("flags")-1, &zv);
		ZVAL_BOOL(&zv, lex.iter_prev->bol);
		zend_hash_str_update(props, "bol", sizeof("bol")-1, &zv);
		ZVAL_LONG(&zv, lex.iter_prev->state);
		zend_hash_str_update(props, "state", sizeof("state")-1, &zv);
		ZVAL_LONG(&zv, lex.iter_prev->first - lex.in.begin());
		zend_hash_str_update(props, "marker", sizeof("marker")-1, &zv);
		ZVAL_LONG(&zv, lex.iter_prev->second - lex.in.begin());
		zend_hash_str_update(props, "cursor", sizeof("cursor")-1, &zv);
	} else {
		ZVAL_LONG(&zv, lex.rules.flags());
		zend_hash_str_update(props, "flags", sizeof("flags")-1, &zv);
		ZVAL_BOOL(&zv, lex.results.bol);
		zend_hash_str_update(props, "bol", sizeof("bol")-1, &zv);
		ZVAL_LONG(&zv, lex.results.state);
		zend_hash_str_update(props, "state", sizeof("state")-1, &zv);
		ZVAL_LONG(&zv, lex.results.first - lex.in.begin());
		zend_hash_str_update(props, "marker", sizeof("marker")-1, &zv);
		ZVAL_LONG(&zv, lex.results.second - lex.in.begin());
		zend_hash_str_update(props, "cursor", sizeof("cursor")-1, &zv);
	}

	return props;
}/*}}}*/

static void
php_parle_lexer_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_lexer_obj *zplo = php_parle_lexer_fetch_obj(obj);
	php_parle_lexer_obj_dtor<ze_parle_lexer_obj>(zplo);
}/*}}}*/

static zend_object *
php_parle_lexer_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_lexer_obj_ctor<ze_parle_lexer_obj, parle::lexer::lexer>(ce);
}/*}}}*/

static zval * 
php_parle_lexer_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	return php_parle_lex_read_property<ze_parle_lexer_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

static void
php_parle_lexer_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	php_parle_lex_write_property<ze_parle_lexer_obj>(object, member, value, cache_slot);
}/*}}}*/

static HashTable * 
php_parle_lexer_get_properties(zval *object) noexcept
{/*{{{*/
	return php_parle_lex_get_properties<ze_parle_lexer_obj>(object);
}/*}}}*/

static void
php_parle_rlexer_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_rlexer_obj *zplo = php_parle_rlexer_fetch_obj(obj);
	php_parle_lexer_obj_dtor<ze_parle_rlexer_obj>(zplo);
}/*}}}*/

static zend_object *
php_parle_rlexer_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	return php_parle_lexer_obj_ctor<ze_parle_rlexer_obj, parle::lexer::rlexer>(ce);
}/*}}}*/

static zval * 
php_parle_rlexer_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	return php_parle_lex_read_property<ze_parle_rlexer_obj>(object, member, type, cache_slot, rv);
}/*}}}*/

static void
php_parle_rlexer_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	php_parle_lex_write_property<ze_parle_rlexer_obj>(object, member, value, cache_slot);
}/*}}}*/

static HashTable * 
php_parle_rlexer_get_properties(zval *object) noexcept
{/*{{{*/
	return php_parle_lex_get_properties<ze_parle_rlexer_obj>(object);
}/*}}}*/

static void
php_parle_parser_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo = php_parle_parser_fetch_obj(obj);

	zend_object_std_dtor(&zppo->zo);

	delete zppo->par;
}/*}}}*/

static zend_object *
php_parle_parser_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo;

	zppo = (ze_parle_parser_obj *)ecalloc(1, sizeof(ze_parle_parser_obj) + zend_object_properties_size(ce));

	zend_object_std_init(&zppo->zo, ce);
	object_properties_init(&zppo->zo, ce);
	zppo->zo.handlers = &parle_parser_handlers;

	zppo->par = new parle::parser::parser{};
	zppo->par->lex = nullptr;

	return &zppo->zo;
}/*}}}*/

static zval * 
php_parle_parser_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo;
	zval tmp_member;
	zval *retval = NULL;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(object));

	auto &par = *zppo->par;

	retval = rv;
	if (strcmp(Z_STRVAL_P(member), "action") == 0) {
		ZVAL_LONG(retval, par.results.entry.action);
	} else if (strcmp(Z_STRVAL_P(member), "reduceId") == 0) {
		try {
			ZVAL_LONG(retval, par.results.reduce_id());
		} catch (const std::exception &e) {
			zend_throw_exception(ParleParserException_ce, e.what(), 0);
		}
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return retval;
}/*}}}*/

static void
php_parle_parser_write_property(zval *object, zval *member, zval *value, void **cache_slot) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo;
	zval tmp_member;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		cache_slot = NULL;
	}

	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(object));

	if (strcmp(Z_STRVAL_P(member), "action") == 0) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Cannot set readonly property $action of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else if (strcmp(Z_STRVAL_P(member), "reduceId") == 0) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Cannot set readonly property $reduceId of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}/*}}}*/

static HashTable * 
php_parle_parser_get_properties(zval *object) noexcept
{/*{{{*/
	ze_parle_parser_obj *zppo;
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);
	zppo = php_parle_parser_fetch_obj(Z_OBJ_P(object));

	auto &par = *zppo->par;

	ZVAL_LONG(&zv, par.results.entry.action);
	zend_hash_str_update(props, "action", sizeof("action")-1, &zv);
	if (par.results.entry.action == parsertl::reduce) {
		ZVAL_LONG(&zv, par.results.reduce_id());
	} else {
		ZVAL_LONG(&zv, Z_L(-1));
	}
	zend_hash_str_update(props, "reduceId", sizeof("reduceId")-1, &zv);

	return props;
}/*}}}*/

static void
php_parle_parser_stack_obj_destroy(zend_object *obj) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso = php_parle_parser_stack_fetch_obj(obj);

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
php_parle_parser_stack_object_init(zend_class_entry *ce) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;

	zpso = (ze_parle_stack_obj *)ecalloc(1, sizeof(ze_parle_stack_obj) + zend_object_properties_size(ce));

	zend_object_std_init(&zpso->zo, ce);
	object_properties_init(&zpso->zo, ce);
	zpso->zo.handlers = &parle_stack_handlers;

	zpso->stack = new parle::stack::stack{};

	return &zpso->zo;
}/*}}}*/

static zval * 
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

	zpso = php_parle_parser_stack_fetch_obj(Z_OBJ_P(object));

	retval = rv;
	if (strcmp(Z_STRVAL_P(member), "top") == 0) {
		if (zpso->stack->empty()) {
			ZVAL_NULL(retval);
		} else {
			ZVAL_COPY(retval, zpso->stack->top());
		}
	} else if (strcmp(Z_STRVAL_P(member), "empty") == 0) {
		ZVAL_BOOL(retval, zpso->stack->empty());
	} else if (strcmp(Z_STRVAL_P(member), "size") == 0) {
		ZVAL_LONG(retval, zpso->stack->size());
	} else {
		retval = (zend_get_std_object_handlers())->read_property(object, member, type, cache_slot, rv);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return retval;
}/*}}}*/

static void
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

	zpso = php_parle_parser_stack_fetch_obj(Z_OBJ_P(object));

	if (strcmp(Z_STRVAL_P(member), "top") == 0) {
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
	} else if (strcmp(Z_STRVAL_P(member), "empty") == 0) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Cannot set readonly property $empty of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else if (strcmp(Z_STRVAL_P(member), "size") == 0) {
		zend_throw_exception_ex(ParleParserException_ce, 0, "Cannot set readonly property $size reduceId of class %s", ZSTR_VAL(Z_OBJ_P(object)->ce->name));
	} else {
		(zend_get_std_object_handlers())->write_property(object, member, value, cache_slot);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}/*}}}*/

static HashTable * 
php_parle_stack_get_properties(zval *object) noexcept
{/*{{{*/
	ze_parle_stack_obj *zpso;
	HashTable *props;
	zval zv;

	props = zend_std_get_properties(object);
	zpso = php_parle_parser_stack_fetch_obj(Z_OBJ_P(object));

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

	memcpy(&parle_lexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_lexer_handlers.clone_obj = NULL;
	parle_lexer_handlers.offset = XtOffsetOf(ze_parle_lexer_obj, zo);
	parle_lexer_handlers.free_obj = php_parle_lexer_obj_destroy;
	parle_lexer_handlers.read_property = php_parle_lexer_read_property;
	parle_lexer_handlers.write_property = php_parle_lexer_write_property;
	parle_lexer_handlers.get_properties = php_parle_lexer_get_properties;
	parle_lexer_handlers.get_property_ptr_ptr = NULL;
	INIT_CLASS_ENTRY(ce, "Parle\\Lexer", ParleLexer_methods);
	ce.create_object = php_parle_lexer_object_init;
	ce.serialize = zend_class_serialize_deny;
	ce.unserialize = zend_class_unserialize_deny;
	ParleLexer_ce = zend_register_internal_class(&ce);
#define DECL_CONST(name, val) zend_declare_class_constant_long(ParleLexer_ce, name, sizeof(name) - 1, val);
	DECL_CONST("ICASE", lexertl::icase)
	DECL_CONST("DOT_NOT_LF", lexertl::dot_not_newline)
	DECL_CONST("DOT_NOT_CRLF", lexertl::dot_not_cr_lf)
	DECL_CONST("SKIP_WS", lexertl::skip_ws)
	DECL_CONST("MATCH_ZERO_LEN", lexertl::match_zero_len)
#undef DECL_CONST
	zend_declare_property_bool(ParleLexer_ce, "bol", sizeof("bol")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleLexer_ce, "flags", sizeof("flags")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleLexer_ce, "state", sizeof("state")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleLexer_ce, "marker", sizeof("marker")-1, Z_L(-1), ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleLexer_ce, "cursor", sizeof("cursor")-1, Z_L(-1), ZEND_ACC_PUBLIC);

	memcpy(&parle_rlexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_rlexer_handlers.clone_obj = NULL;
	parle_rlexer_handlers.offset = XtOffsetOf(ze_parle_rlexer_obj, zo);
	parle_rlexer_handlers.free_obj = php_parle_rlexer_obj_destroy;
	parle_rlexer_handlers.read_property = php_parle_rlexer_read_property;
	parle_rlexer_handlers.write_property = php_parle_rlexer_write_property;
	parle_rlexer_handlers.get_properties = php_parle_rlexer_get_properties;
	parle_rlexer_handlers.get_property_ptr_ptr = NULL;
	INIT_CLASS_ENTRY(ce, "Parle\\RLexer", ParleRLexer_methods);
	//ce.create_object = php_parle_rlexer_object_init;
	ParleRLexer_ce = zend_register_internal_class_ex(&ce, ParleLexer_ce);
	// Hack over do_inherit_parent_constructor, create_object is unconditionally overridden.
	ParleRLexer_ce->create_object = php_parle_rlexer_object_init;

	memcpy(&parle_parser_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_parser_handlers.clone_obj = NULL;
	parle_parser_handlers.offset = XtOffsetOf(ze_parle_parser_obj, zo);
	parle_parser_handlers.free_obj = php_parle_parser_obj_destroy;
	parle_parser_handlers.read_property = php_parle_parser_read_property;
	parle_parser_handlers.write_property = php_parle_parser_write_property;
	parle_parser_handlers.get_properties = php_parle_parser_get_properties;
	parle_parser_handlers.get_property_ptr_ptr = NULL;
	INIT_CLASS_ENTRY(ce, "Parle\\Parser", ParleParser_methods);
	ce.create_object = php_parle_parser_object_init;
	ParleParser_ce = zend_register_internal_class(&ce);
#define DECL_CONST(name, val) zend_declare_class_constant_long(ParleParser_ce, name, sizeof(name) - 1, val);
	DECL_CONST("ACTION_ERROR", parsertl::error)
	DECL_CONST("ACTION_SHIFT", parsertl::shift)
	DECL_CONST("ACTION_REDUCE", parsertl::reduce)
	DECL_CONST("ACTION_GOTO", parsertl::go_to)
	DECL_CONST("ACTION_ACCEPT", parsertl::accept)
	DECL_CONST("ERROR_SYNTAX", parsertl::syntax_error)
	DECL_CONST("ERROR_NON_ASSOCIATIVE", parsertl::non_associative)
	DECL_CONST("ERROR_UNKOWN_TOKEN", parsertl::unknown_token)
#undef DECL_CONST
	zend_declare_property_long(ParleParser_ce, "action", sizeof("action")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleParser_ce, "reduceId", sizeof("reduceId")-1, 0, ZEND_ACC_PUBLIC);
	ParleParser_ce->serialize = zend_class_serialize_deny;
	ParleParser_ce->unserialize = zend_class_unserialize_deny;

	memcpy(&parle_stack_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_stack_handlers.clone_obj = NULL;
	parle_stack_handlers.offset = XtOffsetOf(ze_parle_stack_obj, zo);
	parle_stack_handlers.free_obj = php_parle_parser_stack_obj_destroy;
	parle_stack_handlers.read_property = php_parle_stack_read_property;
	parle_stack_handlers.write_property = php_parle_stack_write_property;
	parle_stack_handlers.get_properties = php_parle_stack_get_properties;
	parle_stack_handlers.get_property_ptr_ptr = NULL;
	INIT_CLASS_ENTRY(ce, "Parle\\Stack", ParleStack_methods);
	ce.create_object = php_parle_parser_stack_object_init;
	ParleStack_ce = zend_register_internal_class(&ce);
	zend_declare_property_bool(ParleStack_ce, "empty", sizeof("empty")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleStack_ce, "size", sizeof("size")-1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(ParleStack_ce, "top", sizeof("top")-1, 0, ZEND_ACC_PUBLIC);
	ParleStack_ce->serialize = zend_class_serialize_deny;
	ParleStack_ce->unserialize = zend_class_unserialize_deny;

	INIT_CLASS_ENTRY(ce, "Parle\\LexerException", NULL);
	ParleLexerException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());
	INIT_CLASS_ENTRY(ce, "Parle\\ParserException", NULL);
	ParleParserException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());
	INIT_CLASS_ENTRY(ce, "Parle\\StackException", NULL);
	ParleStackException_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default());

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
