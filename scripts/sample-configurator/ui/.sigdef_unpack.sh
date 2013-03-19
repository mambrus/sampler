# UI part of rg.sigdef_unpack.sh
# This is not even a script. It is purely meant for being included.

function print_sigdef_unpack_help() {
            cat <<EOF

Usage: ${SIGDEF_UNPACK_SH} [options] [<sigs-file>] [<output-directory>]

Unpacks a sampler singnals description file into a format readable by sample
configurator. Neither sigs-file nor output directory are optional, they have
to be given. If no flag is given, the order is specified above. If one flag
only is given, the remaining argument has to be the corresponding file
or directory.

Options:
  -d          Output directory
  -f          filename for the signals description file
  -h          This help

Example:
  ${SIGDEF_UNPACK_SH} file.ini resdir

EOF
}
	while getopts hd:f: OPTION; do
		case $OPTION in
		h)
			print_sigdef_unpack_help $0
			exit 0
			;;
		d)
			OUT_DIR=$OPTARG
			;;
		f)
			FILE_NAME=$OPTARG
			;;
		?)
			echo "Syntax error:" 1>&2
			print_sigdef_unpack_help $0 1>&2
			exit 2
			;;

		esac
	done
	shift $(($OPTIND - 1))


	FILE_NAME=${FILE_NAME-"${1}"}
	OUT_DIR=${OUT_DIR-"${2}"}

	#sanity checks

	if [ "X${OUT_DIR}" == "X" ]; then
		echo "ERROR: Output directory not specified." 1>&2
		print_sigdef_unpack_help $0 1>&2
		exit 1
	fi
	if [ "X${FILE_NAME}" == "X" ]; then
		echo "ERROR: Signals description file not specified." 1>&2
		print_sigdef_unpack_help $0 1>&2
		exit 1
	fi

	if ! [ -f "${FILE_NAME}" ]; then
		echo "ERROR: Signals description file [${FILE_NAME}] doesn't exist." 1>&2
		exit 1
	fi
	if ! [ -d "${OUT_DIR}" ]; then
		mkdir -p "${OUT_DIR}"
		if ! [ -d "${OUT_DIR}" ]; then
			echo "ERROR: Output directory [${OUT_DIR}] did not exist and"\
				"could not be created." 1>&2
			exit 1
		fi
	fi
