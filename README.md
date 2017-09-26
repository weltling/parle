[![Build Status](https://secure.travis-ci.org/weltling/parle.svg?branch=master)](http://travis-ci.org/weltling/parle)
[![Build status](https://ci.appveyor.com/api/projects/status/w857q34tke5dbt91?svg=true)](https://ci.appveyor.com/project/weltling/parle)

Parle provides lexing and parsing facilities for PHP
=============================================
Lexing and parsing is used widely in the PHP core and extensions. Usually such a functionality is packed into a piece of C/C++ and depends on tools like [flex](http://flex.sourceforge.net/), [re2c](http://re2c.org/), [Bison](http://www.gnu.org/software/bison/), [LEMON](http://www.hwaci.com/sw/lemon/) or similar. With Parle, it is possible to implement lexing and parsing in PHP while relying on features and principles of the parser/lexer generator tools for C/C++. The Lexer and Parser classes are there in the Parle namespace.
The implementation bases on the work of [Ben Hanson](http://www.benhanson.net/)

- https://github.com/BenHanson/lexertl14
- https://github.com/BenHanson/parsertl14

The lexer is based on the pattern matching similar to flex. The parser is LALR(1).

Supported is PHP 7.0 and above. A [C++14](http://en.cppreference.com/w/cpp/compiler_support) capable compiler is required.

The full extension documentation is available in the [PHP Manual](http://php.net/parle).

Installation
============

Read the [INSTALL.md](./INSTALL.md) documentation.


Example tokenizing comma separated integer list
============================================
```php

use Parle\Token;
use Parle\Lexer;
use Parle\LexerException;

/* name => id */
$token = array(
        "COMMA" => 1,
        "CRLF" => 2,
        "DECIMAL" => 3,
);
/* id => name */
$tokenIdToName = array_flip($token);

$lex = new Lexer;
$lex->push("[\x2c]", $token["COMMA"]);
$lex->push("[\r][\n]", $token["CRLF"]);
$lex->push("[\d]+", $token["DECIMAL"]);
$lex->build();

$in = "0,1,2\r\n3,42,5\r\n6,77,8\r\n";

$lex->consume($in);

do {
        $lex->advance();
        $tok = $lex->getToken();

        if (Token::UNKNOWN == $tok->id) {
                throw new LexerException("Unknown token '{$tok->value}' at offset {$lex->marker}.");
        }

        echo "TOKEN: ", $tokenIdToName[$tok->id], PHP_EOL;
} while (Token::EOI != $tok->id);

```


Example parsing comma separated number list
===========================
```php

use Parle\Lexer;
use Parle\Parser;
use Parle\ParserException;

$p = new Parser;
$p->token("CRLF");
$p->token("COMMA");
$p->token("INTEGER");
$p->token("'\"'");
$p->push("START", "RECORDS");
$prod_record_0 = $p->push("RECORDS", "RECORD CRLF");
$prod_record_1 = $p->push("RECORDS", "RECORDS RECORD CRLF");
$prod_int_0 = $p->push("RECORD", "INTEGER");
$prod_int_1 = $p->push("RECORD", "RECORD COMMA INTEGER");
$p->push("DECIMAL", "INTEGER COMMA INTEGER");
$prod_dec_0 = $p->push("RECORD", "'\"' DECIMAL '\"'");
$prod_dec_1 = $p->push("RECORD", "RECORD COMMA '\"' DECIMAL '\"'");
$p->build();

$lex = new Lexer;
$lex->push("[\x2c]", $p->tokenId("COMMA"));
$lex->push("[\r][\n]", $p->tokenId("CRLF"));
$lex->push("[\d]+", $p->tokenId("INTEGER"));
$lex->push("[\x22]", $p->tokenId("'\"'"));
$lex->build();

/* Specifically using comma as both list separator and as a decimal mark. */
$in = "000,111,222\r\n\"333,3\",444,555\r\n666,777,\"888,8\"\r\n";

$p->consume($in, $lex);

do {
	switch ($p->action) {
		case Parser::ACTION_ERROR:
			$err = $p->errorInfo();
			if (Parser::ERROR_UNKOWN_TOKEN == $err->id) {
				$tok = $err->token;
				$msg = "Unknown token '{$tok->value}' at offset {$err->position}";
			} else if (Parser::ERROR_NON_ASSOCIATIVE == $err->id) {
				$tok = $err->token;
				$msg = "Token '{$tok->id}' at offset {$lex->marker}";
			} else if (Parser::ERROR_SYNTAX == $err->id) {
				$tok = $err->token;
				$msg = "Syntax error at offset {$lex->marker}";
			} else {
				$msg = "Parse error";
			}
			throw new ParserException($msg);
			break;
		case Parser::ACTION_SHIFT:
		case Parser::ACTION_GOTO:
		case Parser::ACTION_ACCEPT:
			continue;
			break;
		case Parser::ACTION_REDUCE:
			switch ($p->reduceId) {
				case $prod_int_0:
					/* INTEGER */
					echo $p->sigil(), PHP_EOL;
					break;
				case $prod_int_1:
					/* RECORD COMMA INTEGER */
					echo $p->sigil(2), PHP_EOL;
					break;
				case $prod_dec_0:
					/* '\"' DECIMAL '\"' */
					echo $p->sigil(1), PHP_EOL;
					break;
				case $prod_dec_1:
					/* RECORD COMMA '\"' DECIMAL '\"' */
					echo $p->sigil(3), PHP_EOL;
					break;
				case $prod_record_0:
				case $prod_record_1:
					echo "=====", PHP_EOL;
					break;
			}
			break;
	}
	$p->advance();
} while (Parser::ACTION_ACCEPT != $p->action);

```

