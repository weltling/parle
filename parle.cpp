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

#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
//#include "parsertl/generator.hpp"
//#include "variant.hpp"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "php_parle.h"

#undef lookup

/* If you declare any globals in php_parle.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(parle)
*/

/* True global resources - no need for thread safety here */
/* static int le_parle; */

struct ze_parle_lexer_obj {/*{{{*/
	lexertl::rules *rules;
	lexertl::state_machine *sm;
	lexertl::smatch *results;
	std::string *in;
	uint8_t is_built;
	zend_object zo;
};/*}}}*/

zend_object_handlers parle_lexer_handlers;

static zend_class_entry *ParleLexer_ce;

static zend_always_inline struct ze_parle_lexer_obj *
php_parle_lexer_fetch_obj(zend_object *obj)
{/*{{{*/
	return (struct ze_parle_lexer_obj *)((char *)obj - XtOffsetOf(struct ze_parle_lexer_obj, zo));
}/*}}}*/

/* {{{ public void Lexer::__construct(void) */
PHP_METHOD(ParleLexer, __construct)
{
	struct ze_parle_lexer_obj *zplo;

	/*if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &nsurl, &nsurl_len) == FAILURE) {
		return;
	}*/

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(getThis()));


	/*zplo->rules->push("[0-9]+", 1);
	zplo->rules->push("[a-z]+", 7);
	zplo->rules->push("[A-Z]+", 10);
	zplo->rules->push("[A-Z]{1}[a-z]+", 11);
	zplo->rules->push("[\\s]+", 12);
	lexertl::generator::build(*zplo->rules, *zplo->sm);

	std::string input("abc012Ad3e4 HELLO");
	zplo->results = new lexertl::smatch(input.begin(), input.end());
	lexertl::lookup(*zplo->sm, *zplo->results);

	while (zplo->results->id != 0)
	{
		std::cout << "Id: " << zplo->results->id << ", Token: '" <<
		zplo->results->str () << "'\n";
		lexertl::lookup(*zplo->sm, *zplo->results);
	}*/
}
/* }}} */

/* {{{ public void Lexer::addRule(...) */
PHP_METHOD(ParleLexer, addRule)
{
	struct ze_parle_lexer_obj *zplo;
	zend_string *regex, *regex_start, *regex_end, *dfa, *new_dfa;
	zend_long id, user_id = 0;
	zval *me;

	// Rules for INITIAL
	if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSl|l", &me, ParleLexer_ce, &regex, &id, &user_id) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(regex), id, user_id);
	} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSl|l", &me, ParleLexer_ce, &regex_start, &regex_end, &id, &user_id) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(regex_start), ZSTR_VAL(regex_end), id, user_id);
	// Rules without id
	} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSS", &me, ParleLexer_ce, &dfa, &regex, &new_dfa) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(dfa), ZSTR_VAL(regex), ZSTR_VAL(new_dfa));
	} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSSS", &me, ParleLexer_ce, &dfa, &regex_start, &regex_end, &new_dfa) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(dfa), ZSTR_VAL(regex_start), ZSTR_VAL(regex_end), ZSTR_VAL(new_dfa));
	// Rules with id
	} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSlS|l", &me, ParleLexer_ce, &dfa, &regex, &id, &new_dfa, &user_id) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(dfa), ZSTR_VAL(regex), id, ZSTR_VAL(new_dfa), user_id);
	} else if(zend_parse_method_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), getThis(), "OSSSlS|l", &me, ParleLexer_ce, &dfa, &regex_start, &regex_end, &id, &new_dfa, &user_id) == SUCCESS) {
		zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));
		zplo->rules->push(ZSTR_VAL(dfa), ZSTR_VAL(regex_start), ZSTR_VAL(regex_end), id, ZSTR_VAL(new_dfa), user_id);
	} else {
		zend_throw_exception(zend_ce_exception, "Couldn't match the method signature", 0);
	}
}
/* }}} */

/* {{{ public void Lexer::build(void) */
PHP_METHOD(ParleLexer, build)
{
	struct ze_parle_lexer_obj *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleLexer_ce) == FAILURE) {
		return;
	}

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));

	lexertl::generator::build(*zplo->rules, *zplo->sm);
}
/* }}} */

/* {{{ public void Lexer::consume(string $s) */
PHP_METHOD(ParleLexer, consume)
{
	struct ze_parle_lexer_obj *zplo;
	char *in;
	size_t in_len;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &me, ParleLexer_ce, &in, &in_len) == FAILURE) {
		return;
	}

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));


	zplo->in = new std::string{in};
	zplo->results = new lexertl::smatch(zplo->in->begin(), zplo->in->end());
	lexertl::lookup(*zplo->sm, *zplo->results);
}
/* }}} */

/* {{{ public void Lexer::addState(string $s) */
PHP_METHOD(ParleLexer, addState)
{
	struct ze_parle_lexer_obj *zplo;
	char *state;
	size_t state_len;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &me, ParleLexer_ce, &state, &state_len) == FAILURE) {
		return;
	}

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));


	zplo->rules->push_state(state);
}
/* }}} */

/* {{{ public void Lexer::getToken(void) */
PHP_METHOD(ParleLexer, getToken)
{
	struct ze_parle_lexer_obj *zplo;
	zval *me;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &me, ParleLexer_ce) == FAILURE) {
		return;
	}

	zplo = php_parle_lexer_fetch_obj(Z_OBJ_P(me));

	if (0 == zplo->results->id || zplo->results->npos() == zplo->results->id) {
		RETURN_NULL();
	}

	array_init(return_value);
	add_next_index_long(return_value, zplo->results->id);
	add_next_index_string(return_value, zplo->results->str().c_str());

	lexertl::lookup(*zplo->sm, *zplo->results);
}
/* }}} */

/* {{{ Method and function entries
 */
const zend_function_entry parle_functions[] = {
	PHP_FE_END	/* Must be the last line in parle_functions[] */
};

const zend_function_entry ParleLexer_methods[] = {
	PHP_ME(ParleLexer, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, addRule, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, addState, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, getToken, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, build, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(ParleLexer, consume, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */

void
php_parle_lexer_obj_destroy(zend_object *obj)
{/*{{{*/
	struct ze_parle_lexer_obj *zplo = php_parle_lexer_fetch_obj(obj);

	zend_object_std_dtor(&zplo->zo);

	delete zplo->rules;
	delete zplo->sm;
	delete zplo->results;
	delete zplo->in;
}/*}}}*/

zend_object *
php_parle_lexer_object_init(zend_class_entry *ce)
{/*{{{*/
	struct ze_parle_lexer_obj *zplo;

	zplo = (struct ze_parle_lexer_obj *)ecalloc(1, sizeof(struct ze_parle_lexer_obj));

	zend_object_std_init(&zplo->zo, ce);
	zplo->zo.handlers = &parle_lexer_handlers;

	zplo->rules = new lexertl::rules{};
	zplo->sm = new lexertl::state_machine{};
	zplo->results = nullptr;
	zplo->in = nullptr;

	return &zplo->zo;
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

	memcpy(&parle_lexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	parle_lexer_handlers.clone_obj = NULL;
	parle_lexer_handlers.offset = XtOffsetOf(struct ze_parle_lexer_obj, zo);
	parle_lexer_handlers.free_obj = php_parle_lexer_obj_destroy;

	INIT_CLASS_ENTRY(ce, "Lexer", ParleLexer_methods);
	ce.create_object = php_parle_lexer_object_init;
	ParleLexer_ce = zend_register_internal_class(&ce);

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
	php_info_print_table_header(2, "parle support", "enabled");
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
