#!/bin/bash

# Author: Michael Ambrus (ambrmi09@gmail.com)
# 2013-03-13

if [ -z $SIGDEF_UNPACK_SH ]; then

SIGDEF_UNPACK_SH="sigdef_unpack.sh"

function sigdef_unpack() {
	FS=$(cat "${FILE_NAME}" |  cut -f1 -d";")
	local -i I=0
	for F in $FS; do
		OFNAME="${OUT_DIR}/${I}_${F}.sig"
		echo "Creating file [${OFNAME}]"
		pline_1 "${FILE_NAME}" $(( 1 + $I )) | sed -E 's/;/\n/g' > "${OFNAME}"
		(( I = I + 1 ))
	done
}

source s3.ebasename.sh
if [ "$SIGDEF_UNPACK_SH" == $( ebasename $0 ) ]; then
	#Not sourced, do something with this.

	SIGDEF_UNPACK_SH_INFO=${RDIR_SH}
	source .sc.ui..sigdef_unpack.sh
	source futil.pline.sh
	source s3.user_response.sh
	set -e
	set -u

	sigdef_unpack "$@"
	RC=$?

	exit $RC
fi

fi
