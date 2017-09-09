--TEST--
Lex PHP var statement
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Lexer;
use Parle\Token;

$lex = new Lexer;
$lex->push("\$[a-z]{1,}[a-zA-Z0-9_]+", 1);
$lex->push("=", 2);
$lex->push("[0-9]+", 3);
$lex->push(";", 4);

$lex->build();

$s = "\$hello=42;";
$lex->consume($s);

$lex->advance();
$tok = $lex->getToken();

while (Token::EOI != $tok->id) {
	var_dump($tok);
	$lex->advance();
	$tok = $lex->getToken();
}

?>
==DONE==
--EXPECTF--
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(1)
  ["value"]=>
  string(6) "$hello"
  ["offset"]=>
  int(0)
}
object(Parle\Token)#3 (3) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(1) "="
  ["offset"]=>
  int(6)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(2) "42"
  ["offset"]=>
  int(7)
}
object(Parle\Token)#3 (3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(1) ";"
  ["offset"]=>
  int(9)
}
==DONE==
