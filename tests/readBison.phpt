--TEST--
readBison() test
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 
use Parle\Parser;

$p = new Parser;

$p->readBison("%%\n;start: 'a';%%\n");
$p->dump();

try
{
	$p->readBison("@");
}
catch (\Throwable $e)
{
	echo $e->getMessage(), "\n";
}
?>
==DONE==
--EXPECT--
%token 'a'
%%

start: 'a';

%%
Syntax error on line 1: '@'
==DONE==
