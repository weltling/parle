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

	echo "marker: ", $lex->markerPos, " cursor: ", $lex->cursorPos, "\n";
	$lex->advance();
	$tok = $lex->getToken();

	while (Token::EOI != $tok->id) {
		if ($tok->id > 0) {
			var_dump($tok);
		}
		echo "marker: ", $lex->markerPos, " cursor: ", $lex->cursorPos, "\n";
		$lex->advance();
		$tok = $lex->getToken();
	}
}

?>
==DONE==
--EXPECTF--
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(4)
  ["value"]=>
  string(4) "0x42"
  ["offset"]=>
  int(0)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(1)
  ["value"]=>
  string(8) "0b010101"
  ["offset"]=>
  int(5)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(2)
  ["value"]=>
  string(3) "075"
  ["offset"]=>
  int(14)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(3)
  ["value"]=>
  string(2) "24"
  ["offset"]=>
  int(18)
}
==DONE==
