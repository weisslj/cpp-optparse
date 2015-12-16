#!/bin/bash
# -*- mode: sh; coding: utf-8; indent-tabs-mode: nil -*-
# vim: set filetype=sh fileencoding=utf-8 expandtab sw=4 sts=4:

c () {
    echo "$(printf "%q " ./test "$@")"
    local t_stdout_cpp=$(mktemp --tmpdir cpp-stdout-optparse.XXXXXXXXXX)
    local t_stderr_cpp=$(mktemp --tmpdir cpp-stderr-optparse.XXXXXXXXXX)
    local t_stdout_pyt=$(mktemp --tmpdir pyt-stdout-optparse.XXXXXXXXXX)
    local t_stderr_pyt=$(mktemp --tmpdir pyt-stderr-optparse.XXXXXXXXXX)
    ./test "$@" >"$t_stdout_cpp" 2>"$t_stderr_cpp"
    status_cpp=$?
    ./t/test "$@" >"$t_stdout_pyt" 2>"$t_stderr_pyt"
    status_pyt=$?
    if ! cmp -s "$t_stderr_cpp" "$t_stderr_pyt" ; then
        diff -u "$t_stderr_cpp" "$t_stderr_pyt"
        exit 1
    fi
    rm -f "$t_stderr_cpp" "$t_stderr_pyt"
    if ! cmp -s "$t_stdout_cpp" "$t_stdout_pyt" ; then
        diff -u "$t_stdout_cpp" "$t_stdout_pyt"
        exit 1
    fi
    rm -f "$t_stdout_cpp" "$t_stdout_pyt"
    if [[ $status_cpp -ne $status_pyt ]] ; then
        echo >&2 "status $status_pyt expected, got $status_cpp"
        exit 1
    fi
}

c
c -Z # unknown argument
c --argument-does-not-exist
# c --version # TODO: align formatting
# c --help # TODO: align formatting
c "foo bar" baz ""
c --clear
c --no-clear
c --clear --no-clear
c --clear --no-clear --clear
c --string "foo bar"
c --string # requires argument
c -x foo
c --clause foo
c --sentence foo
c -k -k -k -k -k
c -v
c --verbose
c -s
c --silent
c -v -s
c -n-10
c -n 300
c --number=0
# c -H # TODO: align formatting
# c -V # TODO: align formatting
c -i-10
c -i 300
c --int=0
# c -i 2.3 # TODO: ignores suffix
c -i no-number
c -f-2.3
c -f 300
c --float=0
c -f no-number
c -c-2.3
c -c 300
c --complex=0
c -c no-number
c -C foo
c --choices baz
c -C wrong-choice
c -m a -m b
# c -m a --more b -m c # FIXME: bug
c --more-milk --more-milk
c --hidden foo
# c -K # FIXME: bug
# c -K x # FIXME: bug
c --no-clear foo bar -k -k z -v -n3 "x y" -i 8 -f 3.2 -c 2
DISABLE_INTERSPERSED_ARGS=1 c -k a -k b
