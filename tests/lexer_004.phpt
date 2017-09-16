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
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(4)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(7)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(1) "a"
  ["offset"]=>
  int(8)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(9)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(10)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(13)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(14)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(17)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(5) "again"
  ["offset"]=>
  int(18)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) "
"
  ["offset"]=>
  int(23)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(7) "another"
  ["offset"]=>
  int(24)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(100)
  ["value"]=>
  string(1) " "
  ["offset"]=>
  int(31)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(3) "cmd"
  ["offset"]=>
  int(32)
}
==DONE==
