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

while (RLexer::EOI != $tok["id"]) {
	var_dump($tok);
	$lex->advance();
	$tok = $lex->getToken();
}

?>
==DONE==
--EXPECT--
array(3) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(4)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(7)
}
array(3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(1) "a"
  ["offset"]=>
  int(8)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(9)
}
array(3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(10)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(13)
}
array(3) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(14)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(17)
}
array(3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(5) "again"
  ["offset"]=>
  int(18)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(23)
}
array(3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(7) "another"
  ["offset"]=>
  int(24)
}
array(3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(31)
}
array(3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(32)
}
==DONE==
