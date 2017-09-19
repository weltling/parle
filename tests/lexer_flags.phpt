--TEST--
Lexer flags
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Lexer;
use Parle\Token;

$lex = new Lexer;
$lex->flags = Lexer::DOT_NOT_LF | Lexer::DOT_NOT_CRLF;
var_dump($lex->flags);
$lex->flags = $lex->flags | Lexer::SKIP_WS;
var_dump($lex->flags);

?>
==DONE==
--EXPECTF--
int(6)
int(14)
==DONE==
