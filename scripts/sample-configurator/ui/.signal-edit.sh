# UI part of futil.sample-configurator.signal-edit.sh
# This is not even a script, stupid and can't exist alone. It is purely
# ment for beeing included.

function print_sample-configurator.signal-edit_help() {
			cat <<EOF
Usage:
   ${SAMPLER_SIGNAL_EDIT_SH_INFO} [options]

Edit one sampler signal. Communication must go through physical file as
stout/stderr is occupied and used to control the console.

If no file is given. Finished edit will be outputed on stderr.

Options:
  -f <file>     File to communicate
  -n <num>		Signal number. Informative, can be any string.
  -h            This help

Examples:
  ${SAMPLER_SIGNAL_EDIT_SH_INFO} xpra detach :1234 
  ${SAMPLER_SIGNAL_EDIT_SH_INFO} 'export DISPLAY=\":0.1\" && xpra attach :1234 &'

EOF
}
	while getopts n:f:h OPTION; do
		case $OPTION in
		h)
			clear
			print_sample-configurator.signal-edit_help $0
			exit 0
			;;
		f)
			INOUT_FILE="$OPTARG"
			;;
		n)
			SIGNAL_NUM="$OPTARG"
			;;
		?)
			echo "Syntax error:" 1>&2
			print_sample-configurator.signal-edit_help $0 1>&2
			exit 2
			;;

		esac
	done
	shift $(($OPTIND - 1))

	INOUT_FILE=${INOUT_FILE-""}
	SIGNAL_NUM=${SIGNAL_NUM-"unknown"}

	if [ ! $# -eq 0 ]; then
		echo "Syntax error: $(basename $0) takes o"\
		     "arguments" 1>&2
		echo 1>&2
		print_sample-configurator.signal-edit_help 1>&2
		exit 1
	fi

