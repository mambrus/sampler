#!/bin/bash
# Author: Michael Ambrus (ambrmi09@gmail.com)
# 2013-02-21

if [ -z $SIGNAL_EDIT_SH ]; then

SIGNAL_EDIT_SH="sample-edit.sh"

#1 filename
#2 index (needed due to dialog, usually 0)
function file_to_vars() {
	ptime="$(pline_1 $1 $(( 1 + $2 )) )"
}

function signal_selector() {
	echo "Script not finished (TBD)"
	exit
}

function sc.sample-edit() {

	source futil.pline.sh
	source futil.tmpname.sh
	tmpname_flags_init "-a"

	: ${DIALOG=dialog}
	backtitle="Sample configurator - Sample definition"
	xofs=23
	xmax=0

	if  [ -h $0 ]; then
		#Running installed version.
		PATH_PRFX="$(dirname $0)/sc."
	else
		PATH_PRFX="$(dirname $0)/"
	fi

	MAIN_HLP_FILE="${PATH_PRFX}sample-edit.hlp"

	ptime=""

	if [ "X${INOUT_FILE}" != "X" ]; then
		file_to_vars "${INOUT_FILE}" 0
	fi

	RC=0
	local hfile_supported=$($DIALOG --help 2>&1 | grep -Ee '--hfile\b' > /dev/null && echo yes)
	while [ $RC -ne 1 ] && [ $RC -ne 250 ]; do

		$DIALOG \
			--begin 1 1 \
			--ok-label "Submit" \
			--backtitle "$backtitle" \
			--title "Sample edit" \
			${hfile_supported:+--hfile "${MAIN_HLP_FILE}"} \
			--help-button \
			--help-status \
			--extra-button \
			--extra-label "Abort" \
			--form "Editing signal ID: ${SIGNAL_NUM}" \
		0 0 0 \
			"Period time (uS):"    2 1	"$ptime"  	2 $xofs 80 $xmax \
		2>$(tmpname)
		RC=$?

		VALUES=$(cat $(tmpname))

		hindex=0
		test $RC -eq 2 && (( hindex++ ))

		file_to_vars "$(tmpname)" "$hindex"

		case $RC in
		1)
			$DIALOG \
				--clear \
				--backtitle "$backtitle" \
				--yesno "Really quit?" 10 30
				case $? in
				0)
					break
				;;
			1)
				RC=99
				;;
			esac
			;;
		0)
			signal_selector
			$DIALOG \
				--clear \
				--backtitle "$backtitle" --no-collapse --cr-wrap \
				--msgbox "Resulting data:\n\
$VALUES" 10 40
			if [ "X${INOUT_FILE}" != "X" ]; then
				cat $(tmpname) >  ${INOUT_FILE}
			fi
			#Only normal exit
			exit 0
			
			;;
		2)
			#Button 2 (Help) pressed.
			echo "Data" | $DIALOG \
				--backtitle "$backtitle" \
				--keep-window \
				--no-collapse \
				--cr-wrap \
				--trim \
				--title "Help - Sample edit" \
				--textbox "${MAIN_HLP_FILE}" 20 80
			;;
		3)
			$DIALOG \
				--clear \
				--backtitle "$backtitle" \
				--title "Abort" \
				--yesno "Really abort this signal edit?" 10 30
				case $? in
				0)
					break
				;;
			1)
				RC=99
				;;
			esac
			;;
		*)
			echo "Return code was $RC"
			exit
			;;
		esac
	done

	tmpname_cleanup

	return $RC
}

source s3.ebasename.sh
if [ "$SIGNAL_EDIT_SH" == $( ebasename $0 ) ]; then
	#Not sourced, sample-edit something with this.

	SAMPLER_SIGNAL_EDIT_SH_INFO=${SIGNAL_EDIT_SH}

	source .sc.ui..sample-edit.sh

	tty -s; ATTY="$?"
	ISATTY="$ATTY -eq 0"

	#set -e
	#set -u


	sc.sample-edit "$@"

	RC=$?

	exit $RC
fi

fi

