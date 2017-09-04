[![Build Status](https://secure.travis-ci.org/weltling/parle.svg?branch=master)](http://travis-ci.org/weltling/parle)
[![Build status](https://ci.appveyor.com/api/projects/status/w857q34tke5dbt91?svg=true)](https://ci.appveyor.com/project/weltling/parle)

Parle provides lexing and parsing facilities for PHP
=============================================
Lexing and parsing is used widely in the PHP core and extensions. Usually such a functionality is packed into a PHP extension written in C/C++ and depends on tools like [flex](http://flex.sourceforge.net/), [re2c](http://re2c.org/), [Bison](http://www.gnu.org/software/bison/), [LEMON](http://www.hwaci.com/sw/lemon/) or similar. With Parle, it is possible to implement lexing and parsing in PHP while relying on features and principles of the parser/lexer generator tools for C/C++. The Lexer and Parser classes are there in the Parle namespace.
The implementation bases on the work of [Ben Hanson](http://www.benhanson.net/)

- https://github.com/BenHanson/lexertl14
- https://github.com/BenHanson/parsertl14

Supported is PHP 7.0 and above. A [C++14](http://en.cppreference.com/w/cpp/compiler_support) capable compiler is required.

Example tokenizing comma separated integer list
============================================
```php

use Parle\Lexer;
use Parle\LexerException;

/* name => id */
$token = array(
        "EOI" => 0,
        "COMMA" => 1,
        "CRLF" => 2,
        "DECIMAL" => 3,
);
/* id => name */
$token_rev = array_flip($token);

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

        if ($tok["id"] < 0) {
                throw new LexerException("Unknown token '$tok[value]' at offset $tok[offset].");
        }

        echo "TOKEN: ", $token_rev[$tok["id"]], PHP_EOL;
} while ($lex->eoi() != $tok["id"]);
```


More is to come, stay tuned.
===========================
