dnl $Id$
dnl config.m4 for extension parle

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(parle, for parle support,
dnl Make sure that the comment is aligned:
dnl [  --with-parle             Include parle support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(parle, whether to enable parle support,
dnl Make sure that the comment is aligned:
dnl [  --enable-parle           Enable parle support])

if test "$PHP_PARLE" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-parle -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/parle.h"  # you most likely want to change this
  dnl if test -r $PHP_PARLE/$SEARCH_FOR; then # path given as parameter
  dnl   PARLE_DIR=$PHP_PARLE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for parle files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PARLE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PARLE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the parle distribution])
  dnl fi

  dnl # --with-parle -> add include path
  dnl PHP_ADD_INCLUDE($PARLE_DIR/include)

  dnl # --with-parle -> check for lib and symbol presence
  dnl LIBNAME=parle # you may want to change this
  dnl LIBSYMBOL=parle # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PARLE_DIR/$PHP_LIBDIR, PARLE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PARLELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong parle lib version or lib not found])
  dnl ],[
  dnl   -L$PARLE_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(PARLE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(parle, parle.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
