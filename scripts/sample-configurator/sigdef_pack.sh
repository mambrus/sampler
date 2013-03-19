#!/bin/bash

# Author: Michael Ambrus (ambrmi09@gmail.com)
# 2013-03-13

if [ -z $SIGDEF_PACK_SH ]; then

SIGDEF_PACK_SH="sigdef_pack.sh"

function sigdef_pack() {
	for F in ${IN_DIR}/*sig; do
		(cat $F | awk '{printf("%s;",$0);}'); \
		echo ;
	 done | sed -e 's/;$//' > "${FILE_NAME}"
}

source s3.ebasename.sh
if [ "$SIGDEF_PACK_SH" == $( ebasename $0 ) ]; then
	#Not sourced, do something with this.

	SIGDEF_PACK_SH_INFO=${RDIR_SH}
	source .sample-configurator.ui..sigdef_pack.sh
	source s3.user_response.sh
	set -e
	set -u

	sigdef_pack "$@"
	RC=$?

	exit $RC
fi

fi
