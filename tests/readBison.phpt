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
?>
==DONE==
--EXPECT--
%token 'a'
%%

start: 'a';

%%
==DONE==
