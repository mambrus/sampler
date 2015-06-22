#!/bin/bash

# This script runs on UNIX-hosts to detect Android NDK SYSROOTH PATH
# It takes one optional argument: Full path-name of the compiler.
# Detected SYSROOT is returned on stdout.

SCRIPTNAME=$(basename $(readlink -f $0))

function print_help() {
cat <<EOF
NAME
        ${SCRIPTNAME} - Detect Android NDK SYSROOTH PATH

SYNOPSIS
        ${SCRIPTNAME} [options] gcc_full_pathname

DESCRIPTION
        ${SCRIPTNAME} runs on UNIX-hosts to detect Android NDK SYSROOTH PATH
		It takes one mandatory argument: Full path-name of the compiler.
		Detected SYSROOT is returned on stdout.

EXAMPLES
        ${SCRIPTNAME} /some/install/path/arm-linux-androideabi-gcc
        ${SCRIPTNAME} \$(which mipsel-linux-android-gcc)

OPTIONS
    General options
        -h          This help

    Debugging and verbosity options
        -d          Output additional debugging info and additional
                    verbosity. Prints internal variables.

AUTHOR
        Written by Michael Ambrus, 23 Jun 2015

EOF
}

while getopts hd OPTION; do
	case $OPTION in
	h)
		if [ -t 1 ]; then
			print_help $0 | less -R
		else
			print_help $0
		fi
		exit 0
		;;
	d)
		DEBUG="yes"
		;;
	?)
		echo "Syntax error: options" 1>&2
		echo "For help, type: ${SCRIPTNAME} -h" 1>&2
		exit 2
		;;
	esac
done
shift $(($OPTIND - 1))

function logit() {
	if [ "X${DEBUG}" == "Xyes" ]; then
		echo "$@" 1>&2
	fi
}

#Execute arguments, possibly verbosely
function doit() {
	logit "$@"
	"$@"
}

#Assign variable (arg #1) with value (arg #2), possibly verbosely
function assign() {
	logit "$eval $(echo $1)=$2"
	eval $(echo $1)=$2
}

# - Begin script --------------------------------------------------------------
assign FULL_NAME "${1}"
if ! [ -f "${FULL_NAME}" ]; then
	echo " *** Error ${SCRIPTNAME}: Compiler does not exist:" \
	     "${FULL_NAME} " 1>&2
	exit 1
fi
assign XBIN "$(basename ${FULL_NAME})"

assign COMPILER "$(echo ${XBIN} | sed -e 's/^.*-//')"
if [ "X${COMPILER}" != "Xgcc" ]; then
	echo " *** Error: ${SCRIPTNAME} can only detect SYSROOT for gcc" 1>&2
	exit 1
fi

# Deduct the architecture from the canonical triplet-name of a gcc x-compiler
assign ARCH "$(echo ${XBIN} | sed -e 's/-.*$//')"

# Known Android naming-discrepancies
if [ "X${ARCH}" == "Xmipsel" ]; then
	assign ARCH "mips"
fi
if [ "X${ARCH}" == "Xmips64el" ]; then
	assign ARCH "mips64"
fi
if [ "X${ARCH}" == "Xi686" ]; then
	assign ARCH "x86"
fi

# NDK is installed here. Platforms expected to exist in known subdirectory.
assign NDK_ROOTDIR "$(echo ${FULL_NAME} | sed -e 's/toolchains.*$//')"
if ! [ -d "${NDK_ROOTDIR}" ]; then
	echo " *** Error ${SCRIPTNAME}: Non-existing NDK-tools detected:" \
	     "${NDK_ROOTDIR} " 1>&2
	exit 1
fi

pushd "${NDK_ROOTDIR}/platforms/" > /dev/null
assign LATEST_PLATTFORM "$(ls | sort -n -k2 -t"-" | tail -n1)"
if [ "X${DEBUG}" == "Xyes" ]; then
	echo "Directory is now: $(pwd)"
fi
if ! [ -d "${LATEST_PLATTFORM}/arch-${ARCH}" ]; then
	echo " *** Error ${SCRIPTNAME}: Non-existen directory in latest platform:" \
	     "${LATEST_PLATTFORM}/arch-${ARCH} " 1>&2
	exit 1
fi
doit cd "${LATEST_PLATTFORM}/arch-${ARCH}"
assign SYSROOT $(pwd)
popd > /dev/null

echo "${SYSROOT}"
