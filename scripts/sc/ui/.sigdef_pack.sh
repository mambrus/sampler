# UI part of rg.sigdef_pack.sh
# This is not even a script. It is purely meant for being included.

function print_sigdef_pack_help() {
            cat <<EOF

Usage: ${SIGDEF_PACK_SH} [options] [<output-sigs-file>] [<input-directory>]

Packs a directory with individual singnal description files into sampler
fomat. Neither sigs-file nor output directory are optional, they have
to be given. If no flag is given, the order is specified above. If one flag
only is given, the remaining argument has to be the corresponding file
or directory.


Options:
  -d          Input directory
  -f          Output filename for the resulting signals description file
  -h          This help

Example:
  ${SIGDEF_PACK_SH} file.ini indir

EOF
}
	while getopts hd:f: OPTION; do
		case $OPTION in
		h)
			print_sigdef_pack_help $0
			exit 0
			;;
		d)
			IN_DIR=$OPTARG
			;;
		f)
			FILE_NAME=$OPTARG
			;;
		?)
			echo "Syntax error:" 1>&2
			print_sigdef_pack_help $0 1>&2
			exit 2
			;;

		esac
	done
	shift $(($OPTIND - 1))


	FILE_NAME=${FILE_NAME-"${1}"}
	IN_DIR=${IN_DIR-"${2}"}

	#sanity checks
	if [ "X${IN_DIR}" == "X" ]; then
		echo "ERROR: Output directory not specified." 1>&2
		print_sigdef_pack_help $0 1>&2
		exit 1
	fi
	if [ "X${FILE_NAME}" == "X" ]; then
		echo "ERROR: Signals description file not specified." 1>&2
		print_sigdef_pack_help $0 1>&2
		exit 1
	fi

	if ! [ -d "${IN_DIR}" ]; then
		echo "ERROR: Signals description directory [${IN_DIR}] doesn't exist." 1>&2
		exit 1
	fi
