Parle is a lexer and parsing facility for PHP
=============================================
The Lexer and Parser classes iare there in the Parle namespace.
The implementation bases on the work of [Ben Hanson](http://www.benhanson.net/)

- https://github.com/BenHanson/lexertl14
- https://github.com/BenHanson/parsertl14

Example parsing comma separated integer list
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


More is to come, stay tuned
===========================
