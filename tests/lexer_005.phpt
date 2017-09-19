--TEST--
Lexer marker and cursor
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

$lex->build();

$s = '$x = 42;' . "\n" . '$y;';
$lex->consume($s);

echo "marker: ", $lex->marker, ", cursor: ", $lex->cursor, "\n";
do {
	$lex->advance();
	$tok = $lex->getToken();
	echo "marker: ", $lex->marker, ", cursor: ", $lex->cursor, ", token: '", $tok->value, "'\n";
} while (Token::EOI != $tok->id || $lex->bol);

$len = strlen($s);
if ($lex->cursor == $len) { 
	echo "End of input at ", $len, "\n";
} else {
	echo "End of input should be at ", $len, ", but the cursor is at ", $lex->cursor, "\n";
}
?>
==DONE==
--EXPECTF--
marker: 0, cursor: 0
marker: 0, cursor: 2, token: '$x'
marker: 2, cursor: 3, token: ' '
marker: 3, cursor: 4, token: '='
marker: 4, cursor: 5, token: ' '
marker: 5, cursor: 7, token: '42'
marker: 7, cursor: 8, token: ';'
marker: 8, cursor: 9, token: '
'
marker: 9, cursor: 11, token: '$y'
marker: 11, cursor: 12, token: ';'
marker: 12, cursor: 12, token: ''
End of input at 12
==DONE==
