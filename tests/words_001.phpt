--TEST--
Parse words from a string
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
$lex->push("[^[:blank:][:punct:]]+", $p->tokenId("WORD"));
$lex->push("\\s+", $lex->skip());
$lex->build();


$words = array(
	"Sah ein Knab' ein Röslein stehn",
	"Но, чтобы стоять, я должен держаться корней.",
	"Homines sumus nun dei.",
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
string(31) "Sah ein Knab ein Röslein stehn"
string(76) "Но чтобы стоять я должен держаться корней"
string(21) "Homines sumus nun dei"
==DONE==
