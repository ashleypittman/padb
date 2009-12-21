#!/bin/sh

set -e
set -x

aclocal
autoconf
automake -a
