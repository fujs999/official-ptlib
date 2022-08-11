#!/bin/sh

set -e

PACKAGENAME=ptlib

docker build --build-arg SPECFILE=$PACKAGENAME.spec -t $PACKAGENAME-build --file el7.Dockerfile .
docker run -it --rm --mount type=bind,src=$PWD,dst=/work -w /work $PACKAGENAME-build ./rpmbuild.sh "$@"
