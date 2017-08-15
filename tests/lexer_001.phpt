--TEST--
Lex PHP var statement
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

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

while (0 != $tok["id"]) {
	var_dump($tok);
	$lex->advance();
	$tok = $lex->getToken();
}

?>
==DONE==
--EXPECT--
array(2) {
  ["id"]=>
  int(1)
  ["token"]=>
  string(6) "$hello"
}
array(2) {
  ["id"]=>
  int(2)
  ["token"]=>
  string(1) "="
}
array(2) {
  ["id"]=>
  int(3)
  ["token"]=>
  string(2) "42"
}
array(2) {
  ["id"]=>
  int(4)
  ["token"]=>
  string(1) ";"
}
==DONE==

