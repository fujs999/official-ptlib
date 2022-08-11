#!/bin/bash
# Build script using standard tools (assumes dependencies are already installed)

set -ex

if ! [ -x /usr/bin/spectool ] > /dev/null; then
    echo You must install spectool first: sudo yum install -y rpmdevtools
    exit 1
fi

if [[ -z "$SPECFILE" ]]
then SPECFILE=*.spec   # Expecting only one!
fi

BUILD_ARGS=(--define='_smp_mflags -j2')

if [[ "$BRANCH_NAME" == release/* ]] || [[ "$BRANCH_NAME" == "main" ]]
then BUILD_ARGS+=(--define="branch_id 2" --define="version_stage ReleaseCode")
elif [[ "$BRANCH_NAME" == "develop" ]]
then BUILD_ARGS+=(--define="branch_id 1" --define="version_stage BetaCode")
fi

if [[ $BUILD_NUMBER ]]
then BUILD_ARGS+=(--define="jenkins_release .${BUILD_NUMBER}")
fi

# Create/clean the rpmbuild directory tree
rpmdev-setuptree
rpmdev-wipetree

srcdir=$(rpm --eval "%{_sourcedir}")

# If local sources, need to build/copy them manually
for src in $(spectool --list-files --sources $SPECFILE)
do
    if [[ "$src" == Source*: ]] || [[ "$src" == http* ]]
    then continue
    fi

    if [[ "$src" == *.tar ]]
    then git archive --format=tar --output=$srcdir/$src HEAD
    elif [[ "$src" == *.tgz ]]
    then git archive --format=tar.gz --output=$srcdir/$src HEAD
    else
        dir=$srcdir/$(dirname $src)
        mkdir -p $dir
        cp $src $dir/
    fi
done

# Download the source tarball, copy in our patches
spectool --get-files --sourcedir $SPECFILE
if ls *.patch >/dev/null 2>&1
then cp *.patch $srcdir
fi

# Build the RPM(s)
rpmbuild -bb "$@" "${BUILD_ARGS[@]}" $SPECFILE
