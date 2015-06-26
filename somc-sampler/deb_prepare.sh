#!/bin/bash

#
# Script for use under "debgen"
#
# Build .dpkg using the following command:
# $ mib-tools/debgen/debgen .
#

SAMPLER_VERSION=1.0
SAMPLER_ABI=armeabi-v7a
declare -a SAMPLER_REPOSITORIES
SAMPLER_REPOSITORIES=(\
    "sampler","." \
    "mqueue","libmqueue" \
    )

deb_prepare()
{
    local DEBTEMP="$1";
    local ROOT="$(git rev-parse --show-toplevel)"
    local SUBVERSION=$(date +%Y%m%d).$(git log --since "$(date +%Y-%m-%d) 00:00" --oneline | wc -l)
    local FULL_VERSION=${SAMPLER_VERSION}.${SUBVERSION}
    sed -i -r "s/Version:(.*)/Version: ${FULL_VERSION}/" "$DEBTEMP/DEBIAN/control"

    (cd ${ROOT} && make clean android-clean || true)
    (cd ${ROOT} && make LIB_DYNAMIC=1)
    if [ $? != 0 ] ; then echo 'ERROR in "make"'; return 1 ; fi
    (cd ${ROOT} && make android)
    if [ $? != 0 ] ; then echo 'ERROR in "make android"'; return 1 ; fi

    mkdir -p $DEBTEMP/opt/somc-sampler/bin/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/lib/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/share/etc/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/share/doc/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/share/android/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/share/applications/ && \
    mkdir -p $DEBTEMP/opt/somc-sampler/share/icons/hicolor/scalable/apps/ && \
    mkdir -p $DEBTEMP/usr/bin/ && \
    mkdir -p $DEBTEMP/usr/share/applications/ && \
    mkdir -p $DEBTEMP/usr/share/icons/hicolor/scalable/apps/ && \
    install -p -m 755 ${ROOT}/libs/$SAMPLER_ABI/sampler $DEBTEMP/opt/somc-sampler/share/android/ && \
    install -p -m 644 ${ROOT}/obj/local/$SAMPLER_ABI/sampler $DEBTEMP/opt/somc-sampler/share/android/debug_sampler && \
    install -p -m 755 ${ROOT}/sampler $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 755 ${ROOT}/libmlist/lib/libmlist.so $DEBTEMP/opt/somc-sampler/lib/ && \
    install -p -m 755 ${ROOT}/libsampler/lib/libsampler.so $DEBTEMP/opt/somc-sampler/lib/ && \
    install -p -m 755 ${ROOT}/libmqueue/lib/libmqueue.so $DEBTEMP/opt/somc-sampler/lib/ && \
    install -p -m 644 ${ROOT}/scripts/tests/sampler_data_adaptors.py $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 644 ${ROOT}/scripts/tests/sampler_data_spc.py $DEBTEMP/opt/somc-sampler/bin/ && \
    python ${ROOT}/scripts/tests/sampler_doc.py -i ${ROOT}/doc/1_1_signal_definition.txt -o $DEBTEMP/opt/somc-sampler/bin/sampler_doc_data.py && \
    chmod 644 $DEBTEMP/opt/somc-sampler/bin/sampler_doc_data.py && \
    install -p -m 644 ${ROOT}/scripts/tests/sampler_doc.py $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 644 ${ROOT}/scripts/tests/sampler_editor.ui $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 755 ${ROOT}/scripts/tests/sampler-plot $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 755 ${ROOT}/scripts/tests/sampler-editor $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 755 ${ROOT}/scripts/tests/sampler_merger.py $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 644 ${ROOT}/bin/inserthandlers.py $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 755 ${ROOT}/bin/sampler_insert.py $DEBTEMP/opt/somc-sampler/bin/ && \
    install -p -m 644 ${ROOT}/somc-sampler/somc-sampler-editor.desktop $DEBTEMP/opt/somc-sampler/share/applications/ && \
    install -p -m 644 ${ROOT}/somc-sampler/somc-sampler.svg $DEBTEMP/opt/somc-sampler/share/icons/hicolor/scalable/apps/ && \
    ln -s ../../opt/somc-sampler/bin/sampler-plot $DEBTEMP/usr/bin/sampler-plot && \
    ln -s ../../opt/somc-sampler/bin/sampler-editor $DEBTEMP/usr/bin/sampler-editor && \
    ln -s ../../opt/somc-sampler/bin/sampler_insert.py $DEBTEMP/usr/bin/sampler-insert && \
    ln -s ../../opt/somc-sampler/bin/sampler_merger.py $DEBTEMP/usr/bin/sampler-merger && \
    ln -s ../../../opt/somc-sampler/share/applications/somc-sampler-editor.desktop $DEBTEMP/usr/share/applications/somc-sampler-editor.desktop && \
    ln -s ../../../../../../opt/somc-sampler/share/icons/hicolor/scalable/apps/somc-sampler.svg $DEBTEMP/usr/share/icons/hicolor/scalable/apps/somc-sampler.svg && \
    echo '#/bin/sh' > $DEBTEMP/usr/bin/sampler && \
    echo 'LD_LIBRARY_PATH=/opt/somc-sampler/lib:$LD_LIBRARY_PATH' >> $DEBTEMP/usr/bin/sampler && \
    echo 'export LD_LIBRARY_PATH' >> $DEBTEMP/usr/bin/sampler && \
    echo 'exec /opt/somc-sampler/bin/sampler "$@"' >> $DEBTEMP/usr/bin/sampler && \
    chmod 755 $DEBTEMP/usr/bin/sampler && \
    echo '# Sampler Configuration File' > $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '{' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '  "feedgnuplot": "/usr/bin/somc-feedgnuplot",' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '  "push": [("/opt/somc-sampler/share/android/sampler", "/data/local/tmp/sampler")],' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '  "android_sampler": "/data/local/tmp/sampler",' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '  "host_sampler": "/usr/bin/sampler",' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '  "path_prepend": ["/opt/somc-gnuplot/bin"],' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    echo '}' >> $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    chmod 644 $DEBTEMP/opt/somc-sampler/share/etc/samplerrc && \
    true
    if [ $? != 0 ] ; then return 1 ; fi
    for name in $(cd ${ROOT}/scripts/tests/ && git ls-files *.spc); do
	install -p -m 644 ${ROOT}/scripts/tests/$name $DEBTEMP/opt/somc-sampler/share/etc/
	if [ $? != 0 ] ; then return 1 ; fi
    done
    echo "# git versions for submodules" > $DEBTEMP/opt/somc-sampler/share/doc/VERSIONS
    for repository in "${SAMPLER_REPOSITORIES[@]}"; do
        name="${repository%,*}"
        subdir="${repository#*,}"
        (cd "$ROOT" && cd "$subdir" && echo -n "$name: "; git log -1 "--pretty=format:%h %ai %s%n") \
	    >> $DEBTEMP/opt/somc-sampler/share/doc/VERSIONS
    done
    find $DEBTEMP/ | xargs ls -ld
    return 0
}
