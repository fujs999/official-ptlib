#!/bin/bash
# Build script using standard tools (should be done within a clean VM or
# container to ensure reproducibility, and avoid polluting development environment)

set -e

SPECFILE=bbcollab-ptlib.spec
TARBALL=zsdk-ptlib.src.tgz
BUILD_ARGS=(--define='_smp_mflags -j4')

if [[ $BUILD_NUMBER ]]; then
    BUILD_ARGS+=(--define="jenkins_release .${BUILD_NUMBER}")
fi

if [[ "$BRANCH_NAME" == "develop" ]]; then
    BUILD_ARGS+=(--define="branch_id 1")
elif [[ "$BRANCH_NAME" == "master" ]]; then
    BUILD_ARGS+=(--define="branch_id 2")
fi

if ! which spectool > /dev/null; then
    echo You must install spectool first: sudo yum install -y rpmdevtools
    exit 1
fi

if ! which yum-builddep > /dev/null; then
    echo You must install yum-builddep first: sudo yum install -y yum-utils
    exit 1
fi

# Create/clean the rpmbuild directory tree
rpmdev-setuptree
rpmdev-wipetree

# Update the git commit in revision.h (tarball excludes git repo)
make $(pwd)/revision.h

# Create the source tarball
tar -czf $(rpm --eval "%{_sourcedir}")/$TARBALL --exclude-vcs --exclude=rpmbuild .

# Build the RPM(s)
rpmbuild -ba "${BUILD_ARGS[@]}" $SPECFILE

# Copy the output RPM(s) to the local output directory
mkdir -p rpmbuild
cp -pr $(rpm --eval "%{_rpmdir}") rpmbuild/
