<?php

include_once "parse_str.impl.php";

//$in = "hello=10";
//$in = "hello=10&world=20";
//$in = "hello=10&world[]=20";
//$in = "hello=10&world[my]=20";
//$in = "first=value&arr[0]=foo+bar&arr[]=baz";
//$in = "first=value&arr[0]=foo+bar&arr[]=baz&arr[chk]=";
$in = "hello[bbb][][]=30&world[][]=37&hello[bbb][]=7&foo=10&bar[]=abc+def&empty=";
$in = rtrim(str_repeat($in . "&", 160), "&");

$out0 = array();
$startTime = microtime(true);
Parle\parse_str($in, $out0);
$endTime = microtime(true);
echo 'Took ', (($endTime - $startTime) * 1000), ' milliseconds with parle', "\n";

$out1 = array();
$startTime = microtime(true);
\parse_str($in, $out1);
$endTime = microtime(true);
echo 'Took ', (($endTime - $startTime) * 1000), ' milliseconds with native parse_str', "\n";

//var_dump($out0);
//var_dump($out1);

/* Check same with the random input. */

function rand_in(bool $for_key = false, $len = -1)
{
	$len = $len < 0 ? rand(8, 16) : $len;

	$src = range("a", "z");
	$src = array_merge($src, range(0, 9));
	if (!$for_key) {
		$src = array_merge($src, array("+", "-"));
	}

	$ret = "";
	for ($i = 0; $i < $len; ++$i) {
	    $ret .= $src[mt_rand(0, count($src) - 1)];
	}

	return $ret;
}

$in = rand_in(true) . "[" . rand_in(true) . "][][]=" . rand_in()
	. "&" . rand_in(true) . "[][]=" . rand_in()
	. "&" . rand_in(true) . "[" . rand_in(true) . "][]=" . rand_in()
        . "&" . rand_in(true) . "=" . rand_in()
	. "&" . rand_in(true) . "[]=" . rand_in()
	. "&" . rand_in(true) . "=";
$in = rtrim(str_repeat($in . "&", 160), "&");

$out0 = array();
$startTime = microtime(true);
Parle\parse_str($in, $out0);
$endTime = microtime(true);
echo 'Took ', (($endTime - $startTime) * 1000), ' milliseconds on random input with parle', "\n";

$out1 = array();
$startTime = microtime(true);
\parse_str($in, $out1);
$endTime = microtime(true);
echo 'Took ', (($endTime - $startTime) * 1000), ' milliseconds on random input with native parse_str', "\n";

//var_dump($out0);
//var_dump($out1);


