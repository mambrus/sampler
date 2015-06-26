#!/bin/bash
# Author: Michael Ambrus (ambrmi09@gmail.com)
# 2013-02-20

if [ -z $SIGNAL_EDIT_SH ]; then

SIGNAL_EDIT_SH="signal-edit.sh"

#1 filename
#2 index (needed due to dialog, usually 0)
function file_to_vars() {
	sname="$(pline_1 $1 $(( 1 + $2 )) )"
	nfile="$(pline_1 $1 $(( 2 + $2 )) )"
	dfile="$(pline_1 $1 $(( 3 + $2 )) )"
	fops="$(pline_1  $1 $(( 4 + $2 )) )"
	lpatt="$(pline_1 $1 $(( 5 + $2 )) )"
	lindex="$(pline_1 $1 $(( 6 + $2 )) )"
	spatt="$(pline_1 $1 $(( 7 + $2 )) )"
	indxs="$(pline_1 $1 $(( 8 + $2 )) )"
	nucode="$(pline_1 $1 $(( 9 + $2 )) )"
}

function sc.signal-edit() {

	source futil.pline.sh
	source futil.tmpname.sh
	tmpname_flags_init "-a"

	: ${DIALOG=dialog}
	backtitle="Sample configurator - Signal definition"
	xofs=23
	xmax=0

	if  [ -h $0 ]; then
		#Running installed version.
		PATH_PRFX="$(dirname $0)/sc."
	else
		PATH_PRFX="$(dirname $0)/"
	fi

	MAIN_HLP_FILE="${PATH_PRFX}signal-edit.hlp"

	sname=""
	nfile=""
	dfile=""
	fops=""
	lpatt=""
	lindex=""
	spatt=""
	indxs=""
	nucode=""

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
			--title "Signal edit" \
			${hfile_supported:+--hfile "${MAIN_HLP_FILE}"} \
			--help-button \
			--help-status \
			--extra-button \
			--extra-label "Abort" \
			--form "Editing signal ID: ${SIGNAL_NUM}" \
		0 0 0 \
			"Name (symbolic):"         1 1	"$sname" 	1 $xofs 40 $xmax \
			"Name (from filename):"    2 1	"$nfile"  	2 $xofs 80 $xmax \
			"File:"                    3 1	"$dfile"  	3 $xofs 80 $xmax \
			"File operation (hex):"    4 1	"$fops" 	4 $xofs 4  $xmax \
			"Line pattern:"            5 1	"$lpatt" 	5 $xofs 80 $xmax \
			"Line match index:"        6 1	"$lindex" 	6 $xofs 3  $xmax \
			"Signal pattern:"          7 1	"$spatt" 	7 $xofs 80 $xmax \
			"Mach index(s):"           8 1	"$indxs" 	8 $xofs 20 $xmax \
			"No update behaviour:"     9 1	"$nucode" 	9 $xofs 1  $xmax \
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
				--title "Help - Signal edit" \
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
	#Not sourced, signal-edit something with this.

	SAMPLER_SIGNAL_EDIT_SH_INFO=${SIGNAL_EDIT_SH}

	source .sc.ui..signal-edit.sh

	tty -s; ATTY="$?"
	ISATTY="$ATTY -eq 0"

	#set -e
	#set -u


	sc.signal-edit "$@"

	RC=$?

	exit $RC
fi

fi

