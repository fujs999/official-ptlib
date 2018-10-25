#!/bin/sh

set -e

PACKAGENAME=bbcollab-ptlib
TARGET_OS=${1:-el7}

docker build -f $TARGET_OS.Dockerfile --build-arg SPECFILE=$PACKAGENAME.spec -t $PACKAGENAME-$TARGET_OS-build .
docker run -it --rm --mount type=bind,src=$PWD,dst=/host -w /host $PACKAGENAME-$TARGET_OS-build ./rpmbuild.sh
