--TEST--
Parse words from a string
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\Lexer;
use Parle\Token;

$p = new Parser;
$p->token("WORD");
$p->push("start", "sentence");
$p->push("sentence", "words");
$word_idx = $p->push("words", "WORD");
$words_idx = $p->push("words", "words WORD");
$p->build();

$lex = new Lexer;
$lex->push("[^[:blank:][:punct:]]+", $p->tokenId("WORD"));
$lex->push(".", Token::SKIP);
$lex->build();


$words = array(
	"Sah ein Knab' ein Röslein stehn",
	"Но, чтобы стоять, я должен держаться корней.",
	"Homines sumus nun dei.",
);

foreach ($words as $in) {
	$p->consume($in, $lex);
	$out = array();

	while (Parser::ACTION_ERROR != $p->action && Parser::ACTION_ACCEPT != $p->action) {
		switch ($p->action) {
			case Parser::ACTION_ERROR:
				throw new ParserException("Parser error");
				break;
			case Parser::ACTION_REDUCE:
				switch ($p->reduceId)
				{
					case $word_idx:
						$out[] = $p->sigil(0);
						break;
					case $words_idx:
						$out[] = $p->sigil(1);
						break;
				}
		}

		$p->advance();
	}

	var_dump(implode(" ", $out));
}

?>
==DONE==
--EXPECT--
string(31) "Sah ein Knab ein Röslein stehn"
string(76) "Но чтобы стоять я должен держаться корней"
string(21) "Homines sumus nun dei"
==DONE==
