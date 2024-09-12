#!/bin/sh


to_remove=(
    Makefile.in
    aclocal.m4
    autom4te.cache
    build
    compile
    config.guess
    config.h.in*
    config.sub
    configure
    configure~
    configure.ac~
    depcomp
    dist
    INSTALL
    install-sh
    ltmain.sh
    m4
    missing)


for file in ${to_remove[@]}; do
    find . -path './3rdparty' -prune -o -name "$file" -print0 | xargs -0 rm -rf
    echo "Removed: ${file}"
done


echo "Done."

