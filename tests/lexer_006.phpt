--TEST--
Lexer token callback
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php

use Parle\{Token, Lexer};

$in = '$x =  42;' . "\n\n" . '$y;';

$lex = new Lexer;

$lex->push("\$[a-zA-Z_][a-zA-Z0-9_]*", 1);
$lex->push("=", 2);
$lex->push("\d+", 3);
$lex->push(";", 4);
$lex->push("[ ]", 42);
$lex->callout(42, function () use ($in, $lex) {
	$tok = $lex->getToken();
	echo "Custom handler called, token ", $tok->id, " won't return\n";
	$i = $lex->cursor;
	while (" " == $in[$i]) $i++;
	$lex->reset($i);
	$lex->advance();
});
$f = function () use ($in, $lex)
{
	$tok = $lex->getToken();
	echo "Custom handler called, token ", $tok->id, " won't return\n";
	$i = $lex->cursor;
	while ("\n" == $in[$i]) $i++;
	$lex->reset($i);
	$lex->advance();
};
$lex->push("[\n]", 24);
$lex->callout(24, $f);

$lex->build();

$lex->consume($in);

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
