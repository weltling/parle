--TEST--
Parse words from a string, UTF-8 regex
--SKIPIF--
<?php
if (!extension_loaded("parle")) print "skip";
if (!Parle\INTERNAL_UTF32) print "skip reqire internal UTF-32";
?>
--FILE--
<?php 

use Parle\Parser;
use Parle\Lexer;
use Parle\Token;

$p = new Parser;
$p->token("WORD");
$p->push("start", "sentence");
$p->push("sentence", "words");
$words_idx = $p->push("words", "words WORD");
$word_idx = $p->push("words", "WORD");
$p->build();

$lex = new Lexer;
//$lex->push("[ -\\x10ffff]+", $p->tokenId("WORD"));
$lex->push("[\p{L}\p{P}\p{Lo}]+", $p->tokenId("WORD"));
$lex->push(".", Token::SKIP);
$lex->build();

/* UTF-8 */
$words = array(
	"füße абракадабра 芬蘭",
	"Sah ein Knab' ein Röslein stehn",
	"Но, чтобы стоять, я должен держаться корней.",
	"Homines sumus nun dei.",
	"français éléphant fièvre là où gâteau être île chômage dû Noël maïs aigüe",
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
string(36) "füße абракадабра 芬蘭"
string(32) "Sah ein Knab' ein Röslein stehn"
string(79) "Но, чтобы стоять, я должен держаться корней."
string(22) "Homines sumus nun dei."
string(87) "français éléphant fièvre là où gâteau être île chômage dû Noël maïs aigüe"
==DONE==
