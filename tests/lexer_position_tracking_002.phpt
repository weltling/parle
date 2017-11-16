--TEST--
Lex test line property
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Lexer;
use Parle\Token;

$lex = new Lexer;
$lex->push("[a-z]", 1);
$lex->push("[\n]", 2);
$lex->push("bc", 1);

$lex->build();

$lines = array(
	"abc",
	"d",
	"f",
	"xyz",
);
$s = implode("\n", $lines);
//$s = "abc\nd\n\r\nf\nxyz";
$lex->consume($s);

do {
	$lex->advance();
	$tok = $lex->getToken();
	printf("%d %2d %s\n", $lex->line, $lex->marker, (2 == $tok->id ? ">LF<" : $tok->value));
} while (Token::EOI != $tok->id);

?>
==DONE==
--EXPECTF--
1  0 a
1  1 bc
1  3 >LF<
2  4 d
2  5 >LF<
3  6 f
3  7 >LF<
4  8 x
4  9 y
4 10 z
4 11 
==DONE==
