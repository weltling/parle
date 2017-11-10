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
$lex->push("[\p{L}\p{P}]+", $p->tokenId("WORD"));
$lex->push("[\p{Z}\p{Zl}\p{Zp}\p{Zs}\s]+", Token::SKIP);
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

	$lex->consume($in);

	$lex->advance();
	$tok = $lex->getToken();

	$out = array();
	while (Token::EOI != $tok->id) {
		if ($tok->id > 0) {
			$out[] = $tok->value;
		}
		$lex->advance();
		$tok = $lex->getToken();
	}
	var_dump(implode(" ", $out));
}


?>
==DONE==
--EXPECT--
string(37) "füße абракадабра 芬 蘭"
string(32) "Sah ein Knab' ein Röslein stehn"
string(79) "Но, чтобы стоять, я должен держаться корней."
string(22) "Homines sumus nun dei."
string(87) "français éléphant fièvre là où gâteau être île chômage dû Noël maïs aigüe"
==DONE==
