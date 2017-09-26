--TEST--
Stack var_dump()
--SKIPIF--
<?php if (!extension_loaded("parle")) print "skip"; ?>
--FILE--
<?php

$s = new Parle\Stack;
$s->push(1);
$s->push(2);
$s->push(3);
var_dump($s);
var_dump($s->empty, $s->size, $s->top);

$s->pop();
var_dump($s);

?>
==DONE==
--EXPECTF--
object(Parle\Stack)#%d (4) {
  ["empty"]=>
  bool(false)
  ["size"]=>
  int(3)
  ["top"]=>
  int(3)
  ["elements"]=>
  array(3) {
    [0]=>
    int(3)
    [1]=>
    int(2)
    [2]=>
    int(1)
  }
}
bool(false)
int(3)
int(3)
object(Parle\Stack)#%d (4) {
  ["empty"]=>
  bool(false)
  ["size"]=>
  int(2)
  ["top"]=>
  int(2)
  ["elements"]=>
  array(2) {
    [0]=>
    int(2)
    [1]=>
    int(1)
  }
}
==DONE==
