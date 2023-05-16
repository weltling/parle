--TEST--
readBison() test
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 
use Parle\Parser;
use Parle\Lexer;

$p = new Parser;

$p->readBison("%%\n;start: 'a';%%\n");
$p->build();

$lex = new Lexer;
$lex->push("a", $p->tokenId("'a'"));
$lex->build();

echo $p->validate("a", $lex) . "\n";
?>
==DONE==
--EXPECT--
1
==DONE==
