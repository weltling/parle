--TEST--
Test sigil exceptions
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\RParser;
use Parle\RLexer;
use Parle\Token;


try {
	$p = new RParser;
	$p->sigilName(0);
} catch (\Throwable $e) {
	echo $e->getMessage(), "\n";
}

try {
	$p = new Parser;
	$p->sigil(3);
} catch (\Throwable $e) {
	echo $e->getMessage(), "\n";
}

try {
	$p = new RParser;
	$p->sigilCount();
} catch (\Throwable $e) {
	echo $e->getMessage(), "\n";
}

$p = new RParser;
$p->push("start", "'a'");
$p->build();

$lex = new RLexer;
$lex->push("a", $p->tokenId("'a'"));
$lex->push("\\s+", Token::SKIP);
$lex->build();

$p->consume("a", $lex);

while (Parser::ACTION_ERROR != $p->action && Parser::ACTION_ACCEPT != $p->action) {
	switch ($p->action) {
		case Parser::ACTION_REDUCE:
			// throw here
			echo $p->sigilName(42);
			break;	
	}

	$p->advance();
}
?>
--EXPECTF--
Not in a reduce state!
Not in a reduce state!
Not in a reduce state!

Fatal error: Uncaught Parle\ParserException: Invalid index 42 in %ssigil_002.php:%d
Stack trace:
#0 %ssigil_002.php(45): Parle\RParser->sigilName(42)
#1 {main}
  thrown in %ssigil_002.php on line %d
