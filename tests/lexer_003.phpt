--TEST--
Lex JSON
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php

use Parle\RLexer;
use Parle\Token;

const eOpenOb = 42;
const eCloseOb = 43;
const eOpenArr = 44;
const eCloseArr = 45;
const eName = 46;
const eString = 47;
const eNumber = 48;
const eBoolean = 49;
const eNull = 50;

$lex = new RLexer;

/* UTF-32 */
//$lex->insertMacro("STRING", "[\"]([ -\\x10ffff]{-}[\"\\\\]|\\\\([\"\\\\/bfnrt]|u[0-9a-fA-F]{4}))*[\"]");
/* UTF-8 */
$lex->insertMacro("STRING", "[\"]([ -\x7f]{+}[\x80-\xbf]{+}[\xc2-\xdf]{+}[\xe0-\xef]{+}[\xf0-\xff]{-}[\"\\\]|\\\([\"\\\/bfnrt]|u[0-9a-fA-F]{4}))*[\"]");
$lex->insertMacro("NUMBER", "-?(0|[1-9]\\d*)([.]\\d+)?([eE][-+]?\\d+)?");
$lex->insertMacro("BOOL", "true|false");
$lex->insertMacro("NULL", "null");

$lex->pushState("END");

$lex->pushState("OBJECT");
$lex->pushState("NAME");
$lex->pushState("COLON");
$lex->pushState("OB_VALUE");
$lex->pushState("OB_COMMA");

$lex->pushState("ARRAY");
$lex->pushState("ARR_COMMA");
$lex->pushState("ARR_VALUE");

$lex->push("INITIAL", "[{]", eOpenOb, ">OBJECT:END");
$lex->push("INITIAL", "[[]", eOpenArr, ">ARRAY:END");

$lex->push("OBJECT,OB_COMMA", "[}]", eCloseOb, "<");
$lex->push("OBJECT,NAME", "{STRING}", eName, "COLON");
$lex->push("COLON", ":", Token::SKIP, "OB_VALUE");

$lex->push("OB_VALUE", "{STRING}", eString, "OB_COMMA");
$lex->push("OB_VALUE", "{NUMBER}", eNumber, "OB_COMMA");
$lex->push("OB_VALUE", "{BOOL}", eBoolean, "OB_COMMA");
$lex->push("OB_VALUE", "{NULL}", eNull, "OB_COMMA");
$lex->push("OB_VALUE", "[{]", eOpenOb, ">OBJECT:OB_COMMA");
$lex->push("OB_VALUE", "[[]", eOpenArr, ">ARRAY:OB_COMMA");

$lex->push("OB_COMMA", ",", Token::SKIP, "NAME");

$lex->push("ARRAY,ARR_COMMA", "\\]", eCloseArr, "<");
$lex->push("ARRAY,ARR_VALUE", "{STRING}", eString, "ARR_COMMA");
$lex->push("ARRAY,ARR_VALUE", "{NUMBER}", eNumber, "ARR_COMMA");
$lex->push("ARRAY,ARR_VALUE", "{BOOL}", eBoolean, "ARR_COMMA");
$lex->push("ARRAY,ARR_VALUE", "{NULL}", eNull, "ARR_COMMA");
$lex->push("ARRAY,ARR_VALUE", "[{]", eOpenOb, ">OBJECT:ARR_COMMA");
$lex->push("ARRAY,ARR_VALUE", "[[]", eOpenArr, ">ARRAY:ARR_COMMA");

$lex->push("ARR_COMMA", ",", Token::SKIP, "ARR_VALUE");
$lex->push("*", "[ \t\r\n]+", Token::SKIP, ".");

$lex->build();

$in = file_get_contents(dirname(__FILE__) . DIRECTORY_SEPARATOR . "lexer_003.json");

$lex->consume($in);


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
  int(42)
  ["value"]=>
  string(1) "{"
  ["offset"]=>
  int(0)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(46)
  ["value"]=>
  string(5) ""key""
  ["offset"]=>
  int(5)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(44)
  ["value"]=>
  string(1) "["
  ["offset"]=>
  int(12)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(47)
  ["value"]=>
  string(15) ""qelque choose""
  ["offset"]=>
  int(18)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(48)
  ["value"]=>
  string(2) "42"
  ["offset"]=>
  int(39)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(47)
  ["value"]=>
  string(8) ""füße""
  ["offset"]=>
  int(54)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(45)
  ["value"]=>
  string(1) "]"
  ["offset"]=>
  int(66)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(46)
  ["value"]=>
  string(5) ""obj""
  ["offset"]=>
  int(72)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(42)
  ["value"]=>
  string(1) "{"
  ["offset"]=>
  int(79)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(46)
  ["value"]=>
  string(6) ""prop""
  ["offset"]=>
  int(87)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(48)
  ["value"]=>
  string(2) "12"
  ["offset"]=>
  int(95)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(43)
  ["value"]=>
  string(1) "}"
  ["offset"]=>
  int(101)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(46)
  ["value"]=>
  string(6) ""some""
  ["offset"]=>
  int(107)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(50)
  ["value"]=>
  string(4) "null"
  ["offset"]=>
  int(115)
}
object(Parle\Token)#%d (3) {
  ["id"]=>
  int(43)
  ["value"]=>
  string(1) "}"
  ["offset"]=>
  int(121)
}
==DONE==
