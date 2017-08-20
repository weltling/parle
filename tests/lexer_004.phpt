--TEST--
Restartable lexing
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\RLexer;

$lex = new Parle\RLexer;
$lex->push("can", 1);
$lex->push("^cmd$", 2);
$lex->push("^cmd", 3);
$lex->push("cmd$", 4);
$lex->push("[a-z]+", 50);
$lex->push("\\s+", 100);

$lex->build();

$s = "can\ncmd\na cmd\ncmd again\nanother cmd";
$lex->consume($s);

$lex->restart(4);
$lex->bol(true);

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
  int(2)
  ["token"]=>
  string(3) "cmd"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) "
"
}
array(2) {
  ["id"]=>
  int(50)
  ["token"]=>
  string(1) "a"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) " "
}
array(2) {
  ["id"]=>
  int(4)
  ["token"]=>
  string(3) "cmd"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) "
"
}
array(2) {
  ["id"]=>
  int(3)
  ["token"]=>
  string(3) "cmd"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) " "
}
array(2) {
  ["id"]=>
  int(50)
  ["token"]=>
  string(5) "again"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) "
"
}
array(2) {
  ["id"]=>
  int(50)
  ["token"]=>
  string(7) "another"
}
array(2) {
  ["id"]=>
  int(100)
  ["token"]=>
  string(1) " "
}
array(2) {
  ["id"]=>
  int(4)
  ["token"]=>
  string(3) "cmd"
}
==DONE==
