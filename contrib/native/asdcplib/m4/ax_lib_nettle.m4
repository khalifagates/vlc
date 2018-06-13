# ===========================================================================
#             ax_lib_nettle.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_NETTLE([MINIMUM-VERSION])
#
# DESCRIPTION
#
#   This macro provides tests of availability of Nettle of a
#   particular version or newer. This macros checks for Nettle
#   headers and libraries and defines compilation flags.
#
#   Macro supports following options and their values:
#
#   1) Single-option usage:
#
#     --with-nettle - yes, no or path to Nettle installation prefix
#
#   This macro calls:
#
#     AC_SUBST(NETTLE_CFLAGS)
#     AC_SUBST(NETTLE_LDFLAGS)
#     AC_SUBST(NETTLE_VERSION) - only if version requirement is used
#
#   And sets:
#
#     HAVE_NETTLE
#
# Copyright (c) 2008-2009 CineCert, LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

AC_DEFUN([AX_LIB_NETTLE],
[
    AC_ARG_WITH([nettle],
        AC_HELP_STRING([--with-nettle=@<:@ARG@:>@],
            [use Nettle from given prefix (ARG=path); check standard prefixes (ARG=yes); disable (ARG=no)]
        ),
        [
        if test "$withval" = "yes"; then
            if test -d /var/local/include/nettle ; then
                nettle_prefix=/var/local
            elif test -d /usr/include/nettle ; then
                nettle_prefix=/usr
            else
                nettle_prefix=""
            fi
            nettle_requested="yes"
        elif test -d "$withval"; then
            nettle_prefix="$withval"
            nettle_requested="yes"
        else
            nettle_prefix=""
            nettle_requested="no"
        fi
        ],
        [
        dnl Default behavior is implicit yes
        if test -d /var/local/include/nettle ; then
            nettle_prefix=/var/local
        elif test -d /usr/include/nettle ; then
            nettle_prefix=/usr
        elif test -d /usr/local/include/nettle ; then
            nettle_prefix=/usr/local            
        else
            nettle_prefix=""
        fi
        ]
    )

    NETTLE_CPPFLAGS=""
    NETTLE_LDFLAGS=""
    NETTLE_VERSION=""

    dnl
    dnl Collect include/lib paths and flags
    dnl
    run_nettle_test="no"

    if test -n "$nettle_prefix"; then
        nettle_include_dir="$nettle_prefix/include"
        nettle_ldflags="-L$nettle_prefix/lib64 -L$nettle_prefix/lib"
        run_nettle_test="yes"
    elif test "$nettle_requested" = "yes"; then
        if test -n "$nettle_include_dir" -a -n "$nettle_lib_flags"; then
            run_nettle_test="yes"
        fi
    else
        run_nettle_test="no"
    fi

    nettle_libs="-lnettle -lgmp"

    dnl
    dnl Check Nettle files
    dnl
    if test "$run_nettle_test" = "yes"; then

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -I$nettle_include_dir"

        saved_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $nettle_ldflags"

        saved_LIBS="$LIBS"
        LIBS="$nettle_libs $LIBS"

        dnl
        dnl Check Nettle headers
        dnl
        AC_MSG_CHECKING([for Nettle headers in $nettle_include_dir])

        AC_LANG_PUSH([C])
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM(
                [[
@%:@include <nettle/sha1.h>
@%:@include <nettle/aes.h>
                ]],
                [[]]
            )],
            [
            NETTLE_CPPFLAGS="-I$nettle_include_dir"
            nettle_header_found="yes"
            AC_MSG_RESULT([found])
            ],
            [
            nettle_header_found="no"
            AC_MSG_RESULT([not found])
            ]
        )
        AC_LANG_POP([C])

        dnl
        dnl Check Nettle libraries
        dnl
        if test "$nettle_header_found" = "yes"; then

            AC_MSG_CHECKING([for Nettle libraries])

            AC_LANG_PUSH([C])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM(
                    [[
@%:@include <nettle/aes.h>
@%:@include <nettle/sha1.h>
#if (NETTLE_VERSION_NUMBER < 0x0090700f)
#endif
                    ]],
                    [[

                    ]]
                )],
                [
                NETTLE_LDFLAGS="$nettle_ldflags"
                NETTLE_LIBS="$nettle_libs"
                nettle_lib_found="yes"
                AC_MSG_RESULT([found])
                ],
                [
                nettle_lib_found="no"
                AC_MSG_RESULT([not found])
                ]
            )
            AC_LANG_POP([C])
        fi

        CPPFLAGS="$saved_CPPFLAGS"
        LDFLAGS="$saved_LDFLAGS"
        LIBS="$saved_LIBS"
    fi

    AC_MSG_CHECKING([for Nettle])

    if test "$run_nettle_test" = "yes"; then
        if test "$nettle_header_found" = "yes" -a "$nettle_lib_found" = "yes"; then

            AC_SUBST([NETTLE_CPPFLAGS])
            AC_SUBST([NETTLE_LDFLAGS])
            AC_SUBST([NETTLE_LIBS])

            HAVE_NETTLE="yes"
        else
            HAVE_NETTLE="no"
        fi

        AC_MSG_RESULT([$HAVE_NETTLE])

        dnl
        dnl Check Nettle version
        dnl
        if test "$HAVE_NETTLE" = "yes"; then

            nettle_version_req=ifelse([$1], [], [], [$1])

            if test  -n "$nettle_version_req"; then

                AC_MSG_CHECKING([if Nettle version is >= $nettle_version_req])

                if test -f "$nettle_include_dir/nettle/nettlev.h"; then

                    NETTLE_VERSION=`grep NETTLE_VERSION_TEXT $nettle_include_dir/nettle/nettlev.h \
                                    | grep -v fips | grep -v PTEXT | cut -f 2 | tr -d \"`
                    AC_SUBST([NETTLE_VERSION])

                    dnl Decompose required version string and calculate numerical representation
                    nettle_version_req_major=`expr $nettle_version_req : '\([[0-9]]*\)'`
                    nettle_version_req_minor=`expr $nettle_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
                    nettle_version_req_revision=`expr $nettle_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
                    nettle_version_req_patch=`expr $nettle_version_req : '[[0-9]]*\.[[0-9]]*\.[[0-9]]*\([[a-z]]*\)'`
                    if test "x$nettle_version_req_revision" = "x"; then
                        nettle_version_req_revision="0"
                    fi
                    if test "x$nettle_version_req_patch" = "x"; then
                        nettle_version_req_patch="\`"
                    fi

                    nettle_version_req_number=`expr $nettle_version_req_major \* $((0x10000000)) \
                                               \+ $nettle_version_req_minor \* $((0x100000)) \
                                               \+ $nettle_version_req_revision \* $((0x1000)) \
                                               \+ $((1 + $(printf "%d" \'$nettle_version_req_patch) - $(printf "%d" \'a))) \* $((0x10)) \
                                               \+ $((0xf))`

                    dnl Calculate numerical representation of detected version
                    nettle_version_number=`expr $(($(grep NETTLE_VERSION_NUMBER $nettle_include_dir/nettle/nettlev.h | cut -f 2 | tr -d L)))`

                    nettle_version_check=`expr $nettle_version_number \>\= $nettle_version_req_number`
                    if test "$nettle_version_check" = "1"; then
                        AC_MSG_RESULT([yes])
                    else
                        AC_MSG_RESULT([no])
                        AC_MSG_WARN([Found $NETTLE_VERSION, which is older than required. Possible compilation failure.])
                    fi
                else
                    AC_MSG_RESULT([no])
                    AC_MSG_WARN([Missing header nettle/nettlev.h. Unable to determine Nettle version.])
                fi
            fi
        fi

    else
        HAVE_NETTLE="no"
        AC_MSG_RESULT([$HAVE_NETTLE])

        if test "$nettle_requested" = "yes"; then
            AC_MSG_WARN([Nettle support requested but headers or library not found. Specify valid prefix of Nettle using --with-nettle=@<:@DIR@:>@])
        fi
    fi
    if test "$HAVE_NETTLE" = "yes"; then
        CPPFLAGS="$CPPFLAGS $NETTLE_CPPFLAGS -DHAVE_SSL=1"
        LDFLAGS="$LDFLAGS $NETTLE_LDFLAGS $NETTLE_LIBS"
    fi
])
