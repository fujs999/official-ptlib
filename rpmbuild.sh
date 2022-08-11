#!/bin/bash
# Build script using standard tools (assumes dependencies are already installed)

set -ex

if ! [ -x /usr/bin/spectool ] > /dev/null; then
    echo You must install spectool first: sudo yum install -y rpmdevtools
    exit 1
fi

if [[ -z "$SPECFILE" ]]; then
    SPECFILE=*.spec   # Expecting only one!
fi

BUILD_ARGS=(--define='_smp_mflags -j4')

if [[ "$BRANCH_NAME" == "develop" ]]; then
    BUILD_ARGS+=(--define="branch_id 1")
elif [[ "$BRANCH_NAME" == release/* ]]; then
    BUILD_ARGS+=(--define="branch_id 2")
fi

if [[ $BUILD_NUMBER ]]; then
    BUILD_ARGS+=(--define="jenkins_release .${BUILD_NUMBER}")
fi

# Create/clean the rpmbuild directory tree
rpmdev-setuptree
rpmdev-wipetree

# If local sources, need to build/copy them manually
if [ "`spectool --list-files --source 0 $SPECFILE`" == "Source0: source.tgz" ]; then
    git archive HEAD | gzip > $(rpm --eval "%{_sourcedir}")/source.tgz
fi
if [ "`spectool --list-files --source 1 $SPECFILE`" == "Source1: filter-requires.sh" ]; then
    cp filter-requires.sh $(rpm --eval "%{_sourcedir}")
fi

# Download the source tarball, copy in our patches
spectool --get-files --sourcedir $SPECFILE
if ls *.patch >/dev/null 2>&1; then
    cp *.patch $(rpm --eval "%{_sourcedir}")
fi

# Build the RPM(s)
rpmbuild -ba "$@" "${BUILD_ARGS[@]}" $SPECFILE

# Copy the output RPM(s) to the local output directory
mkdir -p rpmbuild
cp -pr $(rpm --eval "%{_rpmdir}") rpmbuild/
