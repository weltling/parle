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
$lex->push("bc", 3);
$lex->push("ij", 4);

$lex->build();

$lines = array(
	"abc",
	"de",
	"f",
	"ghijk",
	"xyz",
);
$s = implode("\n", $lines);
//$s = "abc\nd\n\r\nf\nxyz";
$lex->consume($s);

printf("L C M  T\n");
do {
	$lex->advance();
	$tok = $lex->getToken();
	printf("%d %d %2d %s\n", $lex->line, $lex->column, $lex->marker, (2 == $tok->id ? ">LF<" : $tok->value));
} while (Token::EOI != $tok->id);

?>
==DONE==
--EXPECTF--
L C M  T
0 0  0 a
0 1  1 bc
0 3  3 >LF<
1 0  4 d
1 1  5 e
1 2  6 >LF<
2 0  7 f
2 1  8 >LF<
3 0  9 g
3 1 10 h
3 2 11 ij
3 4 13 k
3 5 14 >LF<
4 0 15 x
4 1 16 y
4 2 17 z
4 3 18 
==DONE==
