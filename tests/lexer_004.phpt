--TEST--
Restartable lexing
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\RLexer;
use Parle\Token;

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
$lex->bol = true;

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
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(3) "cmd"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(1) "a"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(3) "cmd"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(5) "again"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(7) "another"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
}
==DONE==
