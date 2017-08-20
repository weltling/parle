--TEST--
Parse words from a string, UTF-8 regex
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

use Parle\Parser;
use Parle\Lexer;

$p = new Parser;
$p->token("WORD");
$p->push("start", "sentence");
$p->push("sentence", "words");
$words_idx = $p->push("words", "words WORD");
$word_idx = $p->push("words", "WORD");
$p->build();

$lex = new Lexer;
$lex->push("[ -\\x7f]{+}[\\x80-\\xbf]{+}[\\xc2-\\xdf]{+}[\\xe0-\\xef]{+}[\\xf0-\\xff]+", $p->tokenId("WORD"));
$lex->push("\\s+", $lex->skip());
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
	while (0 != $tok["id"]) {
		if ($tok["id"] > 0) {
			$out[] = $tok["token"];
		}
		$lex->advance();
		$tok = $lex->getToken();
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
