dnl $Id$
dnl config.m4 for extension parle

PHP_ARG_ENABLE(parle, whether to enable parle support,
[  --enable-parle           Enable lexer/parser support])
PHP_ARG_ENABLE(parle-utf32, whether to enable internal UTF-32 support in parle,
[  --enable-parle-utf32     Enable internal UTF-32 support for lexer/parser], no, no)

if test "$PHP_PARLE" != "no"; then
  PHP_REQUIRE_CXX()

  AC_DEFINE(HAVE_PARLE,1,[ ])
  PHP_SUBST(PARLE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(parle, parle.cpp, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++14, cxx)

  PHP_ADD_INCLUDE($ext_srcdir/lib/lexertl14)
  PHP_ADD_INCLUDE($ext_builddir/lib/lexertl14)
  PHP_ADD_INCLUDE($ext_srcdir/lib/parsertl14)
  PHP_ADD_INCLUDE($ext_builddir/lib/parsertl14)
  PHP_ADD_INCLUDE($ext_srcdir/lib/parle)
  PHP_ADD_INCLUDE($ext_builddir/lib/parle)
  PHP_ADD_INCLUDE($ext_srcdir/lib)
  PHP_ADD_INCLUDE($ext_builddir/lib)

  if test "$PHP_PARLE_UTF32" != "no"; then
    AC_DEFINE(HAVE_PARLE_UTF32,1,[ ])
  fi
  dnl PHP_INSTALL_HEADERS([ext/parle/php_parle.h])
fi
