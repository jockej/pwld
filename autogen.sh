#! /bin/sh

aclocal \
    && autoreconf --install \
    && autoconf
