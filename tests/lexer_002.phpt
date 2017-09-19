--TEST--
Lex various number formats
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Lexer;
use Parle\Token;

$lex = new Lexer;

$lex->push("0b[01]+", 1);
$lex->push("0[0-7]*", 2);
$lex->push("[1-9][0-9]*", 3);
$lex->push("0x[0-9a-fA-F]+", 4);

$lex->build();

$nums = array(
	"0x42 0b010101 075 24",
);

foreach ($nums as $in) {

	$lex->consume($in);

	$lex->advance();
	$tok = $lex->getToken();

	while (Token::EOI != $tok->id) {
		if ($tok->id != Token::UNKNOWN) {
			var_dump($tok);
		}
		$lex->advance();
		$tok = $lex->getToken();
	}
}

?>
==DONE==
--EXPECTF--
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(4) "0x42"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(1)
  ["value"]=>
  string(8) "0b010101"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(3) "075"
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(2) "24"
}
==DONE==
