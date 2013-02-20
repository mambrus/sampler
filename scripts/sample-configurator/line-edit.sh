#!/bin/bash
source futil.pline.sh
source futil.tmpname.sh
tmpname_flags_init "-a"

: ${DIALOG=dialog}
backtitle="Sample configuration - Signal definition"
xofs=23
xmax=0


sname=	""
nfile=	""
dfile=	""
pers=	""
lpatt=	""
spatt=	""
indxs=	""

LINE_NUM=0

#	--form <text> \
#<height> <width> <form height> \
#	<label1> <l_y1> <l_x1> <item1> <i_y1> <i_x1> <flen1> <ilen1> \

RC=0

while [ $RC -ne 1 ] && [ $RC -ne 250 ]; do
	# open fd
	exec 3>&1

	# Store data to $VALUES variable
	#VALUES=$($DIALOG \
	$DIALOG \
		--begin 1 1 \
		--ok-label "Submit" \
		--backtitle "$backtitle" \
		--title "Signal edit" \
		--hfile "line-edit-help.txt" \
		--help-button \
		--form "Editing signal: ${LINE_NUM}" \
	0 0 0 \
		"Name (symbolic):"         1 1	"$sname" 	1 $xofs 40 $xmax \
		"Name (from filename):"    2 1	"$nfile"  	2 $xofs 80 $xmax \
		"File:"                    3 1	"$dfile"  	3 $xofs 80 $xmax \
		"Persistance:"             4 1	"$pers" 	4 $xofs 1 $xmax \
		"Line pattern:"            5 1	"$lpatt" 	5 $xofs 80 $xmax \
		"Signal pattern:"          6 1	"$spatt" 	6 $xofs 80 $xmax \
		"Mach index(s):"           7 1	"$indxs" 	7 $xofs 20 $xmax \
	2>$(tmpname)
	
	#2>&1 1>&3)

	RC=$?

	# close fd
	exec 3>&-

	VALUES=$(cat $(tmpname))

	sname="$(pline_1 $(tmpname) 1)"
	nfile="$(pline_1 $(tmpname) 2)"
	dfile="$(pline_1 $(tmpname) 3)"
	pers="$(pline_1  $(tmpname) 4)"
	lpatt="$(pline_1 $(tmpname) 5)"
	spatt="$(pline_1 $(tmpname) 6)"
	indxs="$(pline_1 $(tmpname) 7)"


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
		;;
	2)
		#Button 2 (Help) pressed.
		$DIALOG \
			--backtitle "$backtitle" \
			--keep-window \
			--no-collapse \
			--cr-wrap \
			--trim \
			--title "Help - One line edit" \
			--textbox line-edit-help.txt 20 80
		;;
	3)
		echo "Button 3 (Extra) pressed."
		exit
		;;
	*)
		echo "Return code was $RC"
		exit
		;;
	esac

done

echo "$VALUES"

tmpname_cleanup
