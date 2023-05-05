--TEST--
Lexer functionality while it's used by parser
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--XFAIL--
Different exception text depending on PHP version. Need to fix the test.
--FILE--
<?php

use Parle\{Parser, Lexer, Token, ParserException};

$par = new Parser;
$par->token("NEWLINE");
$par->token("LETTER");
$par->token("' '");
$par->push("START", "LETTERS");
$prod_0 = $par->push("LETTERS", "LETTER");
$prod_1 = $par->push("LETTERS", "LETTERS LETTER");
$par->push("LETTERS", "LETTERS NEWLINE LETTERS");
$par->push("LETTERS", "LETTERS NEWLINE");
$par->build();


$lex = new Lexer;
$lex->push("[a-z]", $par->tokenId("LETTER"));
$lex->push("[\r]?[\n]", $par->tokenId("NEWLINE"));
$lex->build();

$in = "abc\ndef\r\nghf\nxy\\z";
$par->consume($in, $lex);


do {
	switch ($par->action) {
	case Parser::ACTION_ERROR:
		$i = $par->errorInfo();
		var_dump($i, $lex, $lex->getToken(), substr($in, $lex->marker, $lex->cursor - $lex->marker));
		throw new ParserException("Error");
		break;
	/*case Parser::ACTION_GOTO:
		echo "Trace: '", $par->trace(), "', token: '", $lex->getToken()->value, "'", PHP_EOL;
		break;
	case Parser::ACTION_SHIFT:
		echo "Trace: '", $par->trace(), "', token: '", $lex->getToken()->value, "'", PHP_EOL;
		break;*/
	case Parser::ACTION_REDUCE:
		//echo "Trace: ", $par->trace(), PHP_EOL;
		switch ($par->reduceId) {
			case $prod_0;
				echo " Match: '", $par->sigil(0), "', token: '", substr($in, $lex->marker, $lex->cursor - $lex->marker), "'", PHP_EOL;
				break;
			case $prod_1;
				echo " Match: '", $par->sigil(1), "', token: '", substr($in, $lex->marker, $lex->cursor - $lex->marker), "'", PHP_EOL;
				break;
		}
		break;
	}
	$par->advance();
} while (Parser::ACTION_ACCEPT != $par->action);

?>
==DONE==
--EXPECTF--
Match: 'a', token: 'b'
 Match: 'b', token: 'c'
 Match: 'c', token: '
'
 Match: 'd', token: 'e'
 Match: 'e', token: 'f'
 Match: 'f', token: '
'
 Match: 'g', token: 'h'
 Match: 'h', token: 'f'
 Match: 'f', token: '
'
 Match: 'x', token: 'y'
object(Parle\ErrorInfo)#%d (3) {
  ["id"]=>
  int(2)
  ["position"]=>
  int(15)
  ["token"]=>
  object(Parle\Token)#4 (2) {
    ["id"]=>
    int(65535)
    ["value"]=>
    string(1) "\"
  }
}
object(Parle\Lexer)#%d (7) {
  ["bol"]=>
  bool(false)
  ["flags"]=>
  int(6)
  ["state"]=>
  int(0)
  ["marker"]=>
  int(15)
  ["cursor"]=>
  int(16)
  ["line"]=>
  int(2)
  ["coulmn"]=>
  int(2)
}
object(Parle\Token)#%d (2) {
  ["id"]=>
  int(65535)
  ["value"]=>
  string(1) "\"
}
string(1) "\"

Fatal error: Uncaught Parle\ParserException: Error in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
