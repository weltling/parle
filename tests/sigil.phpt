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
$b_idx = $p->push("B", "'b'");
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
			echo $p->sigilName(($p->reduceId == $b_idx) ? 0 : 1) . "\n";
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
B
2
==DONE==
