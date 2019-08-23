--TEST--
return type in arg info
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php 

$r = new ReflectionMethod("Parle\\Lexer", "getToken");
var_dump(PHP_VERSION_ID >= 70100 ? $r->getReturnType()->getName() : (string)$r->getReturnType());
$r = new ReflectionMethod("Parle\\RLexer", "getToken");
var_dump(PHP_VERSION_ID >= 70100 ? $r->getReturnType()->getName() : (string)$r->getReturnType());
$r = new ReflectionMethod("Parle\\Parser", "errorInfo");
var_dump(PHP_VERSION_ID >= 70100 ? $r->getReturnType()->getName() : (string)$r->getReturnType());
$r = new ReflectionMethod("Parle\\RParser", "errorInfo");
var_dump(PHP_VERSION_ID >= 70100 ? $r->getReturnType()->getName() : (string)$r->getReturnType());

?>
==DONE==
--EXPECTF--
string(11) "Parle\Token"
string(11) "Parle\Token"
string(15) "Parle\ErrorInfo"
string(15) "Parle\ErrorInfo"
==DONE==
