dnl $Id$
dnl config.m4 for extension parle

PHP_ARG_ENABLE(parle, whether to enable parle support,
[  --enable-parle           Enable lexer/parser support])

if test "$PHP_PARLE" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY(stdc++,,PARLE_SHARED_LIBADD)

  AC_DEFINE(HAVE_PARLE,1,[ ])
  PHP_SUBST(PARLE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(parle, parle.cpp, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++14)

  PHP_ADD_INCLUDE($ext_srcdir/lib/lexertl14)
  PHP_ADD_INCLUDE($ext_builddir/lib/lexertl14)
  PHP_ADD_INCLUDE($ext_srcdir/lib/parsertl14)
  PHP_ADD_INCLUDE($ext_builddir/lib/parsertl14)
  PHP_ADD_INCLUDE($ext_srcdir/lib/mpark)
  PHP_ADD_INCLUDE($ext_builddir/lib/mpark)

  PHP_INSTALL_HEADERS([ext/parle/php_parle.h])
fi
