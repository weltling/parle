--TEST--
Advanced calc with state
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\Stack;
use Parle\ParserException;
use Parle\Lexer;
use Parle\Token;

$p = new Parser;
$p->token("INTEGER");
$p->left("'+' '-' '*' '/'");
$p->precedence("NEGATE");
$p->right("'^'");

$p->push("start", "exp");
$add_idx = $p->push("exp", "exp '+' exp");
$sub_idx = $p->push("exp", "exp '-' exp");
$mul_idx = $p->push("exp", "exp '*' exp");
$div_idx = $p->push("exp", "exp '/' exp");
$p->push("exp", "'(' exp ')'");
$neg_idx = $p->push("exp", "'-' exp %prec NEGATE");
$exp_idx = $p->push("exp", "exp '^' exp");
$int_idx = $p->push("exp", "INTEGER");

$p->build();

$lex = new Lexer;
$lex->push("[+]", $p->tokenId("'+'"));
$lex->push("[-]", $p->tokenId("'-'"));
$lex->push("[*]", $p->tokenId("'*'"));
$lex->push("[\\^]", $p->tokenId("'^'"));
$lex->push("[/]", $p->tokenId("'/'"));
$lex->push("\\d+", $p->tokenId("INTEGER"));
$lex->push("[(]", $p->tokenId("'('"));
$lex->push("[)]", $p->tokenId("')'"));
$lex->push("\\s+", 42);
$lex->setCallback(42, function () use ($lex) {
	do {
		$lex->advance();
		$tok = $lex->getToken();
	} while (42 == $tok->id);
});

$lex->build();

$exp = array(
	"1 + 2^4",
	"33 / (10 + 1)",
	"100 * 45 / 10",
	"10*5 - 45",
	"10 - -4",
	"10000000^0 + 10 - 3^2",
);

foreach ($exp as $in) {
	if (!$p->validate($in, $lex)) {
		throw new ParserException("Failed to validate input");
	}

	$p->consume($in, $lex);

	$stack = new Stack;

	while (Parser::ACTION_ERROR != $p->action && Parser::ACTION_ACCEPT != $p->action) {
		switch ($p->action) {
			case Parser::ACTION_ERROR:
				throw new ParserException("Parser error");
				break;
			case Parser::ACTION_SHIFT:
			case Parser::ACTION_GOTO:
			case Parser::ACTION_ACCEPT:
				break;
			case Parser::ACTION_REDUCE:
				switch ($p->reduceId) {
					case $add_idx:
						$op0 = $stack->top;
						$stack->pop();
						$stack->top += $op0;
						break;
					case $sub_idx:
						$op0 = $stack->top;
						$stack->pop();
						$stack->top -= $op0;
						break;
					case $mul_idx:
						$op0 = $stack->top;
						$stack->pop();
						$stack->top *= $op0;
						break;
					case $div_idx:
						$op0 = $stack->top;
						$stack->pop();
						$stack->top /= $op0;
						break;
					case $exp_idx:
						$op0 = $stack->top;
						$stack->pop();
						$stack->top = $stack->top ** $op0;
						break;
					case $neg_idx:
						$stack->top = -$stack->top;
						break;
					case $int_idx:
						$i = (int)$p->sigil();
						$stack->push($i);
						break;
				}

			break;
		}
		$p->advance();
	}
	echo "$in = " . $stack->top . "\n";
}

?>
==DONE==
--EXPECT--
1 + 2^4 = 17
33 / (10 + 1) = 3
100 * 45 / 10 = 450
10*5 - 45 = 5
10 - -4 = 14
10000000^0 + 10 - 3^2 = 2
==DONE==
