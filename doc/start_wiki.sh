#!/bin/bash

#Allow script to be started from any-whare (including if it's a link)
pushd $(dirname $(readlink -f $0)) 1>/dev/null

#Assume project-name is the same as base directory.
PROJECT=$(basename $(cd $(pwd)/..; pwd))
#                              ^---- Please note if copied as template

if [ -f ~/.cabal/bin/gitit ]; then
	echo "Will start [${PROJECT}] with local-build gitit binary... (preferred)"
	GITIT_BIN=~/.cabal/bin/gitit
else
	echo "Will start [${PROJECT}] with system installed gitit binary..."
	GITIT_BIN=$(which gitit)
fi

if [ "X${GITIT_BIN}" == "X" ]; then
	echo -n "ERROR: Can't start. No gitit found, neither Cabal-built or " 1>&2
	echo "system-installed." 1>&2
	exit 1
fi

#get config port:
CPORT=$(grep -e'^port:[[:space:]]' wiki_config.txt | cut -f2 -d" ")

if [ "X$(screen -ls | grep wiki-${PROJECT})" != "X" ]; then
	echo 1>&2
	echo "WARN: Service is already started and starting will fail." 1>&2
	echo "To restart, kill old instance first..." 1>&2
	echo "Enter local screen as follows, terminate, " \
	     "then re-run this script:" 1>&2
	echo "  screen -rd \"wiki-${PROJECT}\"" 1>&2
	echo 1>&2
fi

if [ "X$(which screen 2>/dev/null)" == "X" ]; then
	echo "Error: screen needed to run service. Please install..."
	exit 1
fi

echo "Gitit starting local webserver at http://127.0.0.1:${CPORT} in screen"
screen -dmS "wiki-${PROJECT}" ${GITIT_BIN} -f wiki_config.txt -l 127.0.0.1
echo "To enter local screen:"
echo "  screen -rd \"wiki-${PROJECT}\""

popd 1>/dev/null
