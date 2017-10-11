--TEST--
Test lexer/parser argument checking
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\RParser;
use Parle\ParserException;
use Parle\Lexer;
use Parle\RLexer;

$in = "1 + 1";

// variation 0
try {
	$p = new Parser;
	$p->token("INTEGER");
	$p->push("start", "exp");
	$int_idx = $p->push("exp", "INTEGER");
	$p->build();

	$lex = new RLexer;
	$lex->push("\\d+", $p->tokenId("INTEGER"));
	$lex->build();


} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}

try {
	$p->validate($in, $lex);
} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}

try {
	$p->consume($in, $lex);
} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}


// variation 1
try {
	$p = new RParser;
	$p->token("INTEGER");
	$p->push("start", "exp");
	$int_idx = $p->push("exp", "INTEGER");
	$p->build();

	$lex = new Lexer;
	$lex->push("\\d+", $p->tokenId("INTEGER"));
	$lex->build();


} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}

try {
	$p->validate($in, $lex);
} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}

try {
	$p->consume($in, $lex);
} catch (\Throwable $e) {
	echo $e->getMessage(), PHP_EOL;
}

?>
==DONE==
--EXPECT--
Argument 2 passed to Parle\Parser::validate() must be an instance of Parle\Lexer, instance of Parle\RLexer given
Argument 2 passed to Parle\Parser::consume() must be an instance of Parle\Lexer, instance of Parle\RLexer given
Argument 2 passed to Parle\RParser::validate() must be an instance of Parle\RLexer, instance of Parle\Lexer given
Argument 2 passed to Parle\RParser::consume() must be an instance of Parle\RLexer, instance of Parle\Lexer given
==DONE==
