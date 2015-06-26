#!/bin/bash

#Installs desktop launchers for signal/sample-configurator (sc) for Gnome
#Run this from root directory, i.e. ./scripts/sc/

#References:
#https://bbs.archlinux.org/viewtopic.php?id=122033
#http://askubuntu.com/questions/67382/add-custom-command-in-the-open-with-dialog

FS=$(ls desktop/*.desktop)
for F in $FS; do 
	echo "Installing Gnome desktop for [$F]"
    cp $F ${HOME}/.local/share/applications/
done


