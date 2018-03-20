#!/bin/sh

set -e

PACKAGENAME=bbcollab-ptlib

docker build --build-arg SPECFILE=$PACKAGENAME.spec -t $PACKAGENAME-build .
docker run -it --rm --mount type=bind,src=$PWD,dst=/host -w /host $PACKAGENAME-build ./rpmbuild.sh
