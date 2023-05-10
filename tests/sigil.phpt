--TEST--
Test sigil functions
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\Lexer;
use Parle\Token;
use Parle\SigilToken;

$p = new Parser;
$p->push("start", "'a' B");
$p->push("B", "'b'");
$p->build();

$lex = new Lexer;
$lex->push("a", $p->tokenId("'a'"));
$lex->push("b", $p->tokenId("'b'"));
$lex->push("\\s+", Token::SKIP);
$lex->build();

$p->consume("a b", $lex);

while (Parser::ACTION_ERROR != $p->action && Parser::ACTION_ACCEPT != $p->action) {
	switch ($p->action) {
		case Parser::ACTION_REDUCE:
			echo $p->sigilInfo(0)->name . "\n";
			echo $p->sigilInfo(0)->token . "\n";
			echo $p->sigilCount() . "\n";
			break;	
	}

	$p->advance();
}
?>
==DONE==
--EXPECT--
'b'
1
1
'a'
1
2
==DONE==
