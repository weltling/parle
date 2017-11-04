--TEST--
Lexer token callback
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php

use Parle\{Token, Lexer};

$lex = new Lexer;

$lex->push("\$[a-zA-Z_][a-zA-Z0-9_]*", 1);
$lex->push("=", 2);
$lex->push("\d+", 3);
$lex->push(";", 4);
$lex->push("[ ]+", 42);
$lex->callout(42, function () use ($lex) {
	echo "Custom handler called, token ", $lex->getToken()->id, " won't return\n";
	do {
		$lex->advance();
		$tok = $lex->getToken();
	} while (42 == $tok->id);
});
$f = function () use ($lex)
{
	echo "Custom handler called, token ", $lex->getToken()->id, " won't return\n";
	do {
		$lex->advance();
		$tok = $lex->getToken();
	} while (24 == $tok->id);
};
$lex->push("[\n]+", 24);
$lex->callout(24, $f);

$lex->build();

$s = '$x = 42;' . "\n" . '$y;';
$lex->consume($s);

do {
	$lex->advance();
	$tok = $lex->getToken();
	echo $tok->id, "\n";
} while (Token::EOI != $tok->id);

?>
==DONE==
--EXPECT--
1
Custom handler called, token 42 won't return
2
Custom handler called, token 42 won't return
3
4
Custom handler called, token 24 won't return
1
4
0
==DONE==
