<?php

/*
  Mimic benchmarks from https://github.com/nikic/Phlexy 
  Care about patterns, not 100% matching with PCRE, same functionality.
  Also, including all the creation time, too.
 */

use Parle\Lexer;
use Parle\RLexer;
use Parle\Token;

// array(id, reg)
$csvDefs = array(
	array(1, "[^\"\,\r\n]+"),
	array(2, "[\"][^\"]+[\"]"),
	array(3, "[,]"),
	array(4, "[\r]?[\n]"),
);
$alphabet = range('a', 'z');
$alphabetDefs = array();
foreach($alphabet as $c) {
	$alphabetDefs[] = array(ord($c), "[" . $c . "]");
}

$cvsString = trim(str_repeat('hallo world,foo bar,more foo,more bar,"rare , escape",some more,stuff' . "\n", 5000));
$allAString = str_repeat('a', 100000);
$allZString = str_repeat('z', 20000);
$randomString = '';
for ($i = 0; $i < 50000; ++$i) {
    $randomString .= $alphabet[mt_rand(0, count($alphabet) - 1)];
}

echo 'Timing lexing of CSV data:', "\n";
$lex = new Lexer;
testLexingPerformance($lex, $csvDefs, $cvsString);
$lex = new RLexer;
testLexingPerformance($lex, $csvDefs, $cvsString);
echo "\n";


echo 'Timing alphabet lexing of all "a":', "\n";
$lex = new Lexer;
testLexingPerformance($lex, $alphabetDefs, $allAString);
$lex = new RLexer;
testLexingPerformance($lex, $alphabetDefs, $allAString);
echo "\n";


echo 'Timing alphabet lexing of all "z":', "\n";
$lex = new Lexer;
testLexingPerformance($lex, $alphabetDefs, $allZString);
$lex = new RLexer;
testLexingPerformance($lex, $alphabetDefs, $allZString);
echo "\n";


echo 'Timing alphabet lexing of random string:', "\n";
$lex = new Lexer;
testLexingPerformance($lex, $alphabetDefs, $randomString);
$lex = new RLexer;
testLexingPerformance($lex, $alphabetDefs, $randomString);
echo "\n";

function testLexingPerformance($lex, $defs, $in)
{
	$startTime = microtime(true);

	foreach($defs as $d) {
		$lex->push($d[1], $d[0]);
	}
	$lex->build();
	$lex->consume($in);

	do {
		$lex->advance();
		$tok = $lex->getToken();
//		var_dump($tok);
	} while (Token::EOI != $tok->id);

	$endTime = microtime(true);

	echo 'Took ', $endTime - $startTime, ' seconds (', get_class($lex), ')', "\n";
}
