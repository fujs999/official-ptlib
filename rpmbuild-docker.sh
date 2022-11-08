#!/bin/sh

set -e

SPECFILE=$(ls *.spec)
DIST="amzn2022"
ARCH=$(uname -m | sed -e 's/arm64/aarch64/' -e 's/x86_64/amd64/')
COMMAND="./rpmbuild.sh"

while [[ $# -gt 0 ]]; do
  case $1 in
    -f|--specfile)
      SPECFILE="$2"
      shift # argument
      shift # value
      ;;
    -d|--dist)
      DIST="$2"
      shift # argument
      shift # value
      ;;
    -a|--arch)
      ARCH="$2"
      shift # argument
      shift # value
      ;;
    -c|--command)
      COMMAND="$2"
      shift # argument
      shift # value
      ;;
    *)
      break # pass remaining args to the docker run
      ;;
  esac
done

if [ -z "$SPECFILE" ]; then
  echo "No .spec file!"
  exit 1
fi

if [ ! -e "$DIST.Dockerfile" ]; then
  echo "Cannot use distro as no $DIST.Dockerfile file!"
  exit 1
fi

DOCKER_TAG="${SPECFILE/.spec/}-$DIST-$ARCH-builder"

echo "Building $DIST/$ARCH docker image \"$DOCKER_TAG\" for \"$SPECFILE\""
docker build \
    --progress plain \
    --platform $ARCH \
    --build-arg SPECFILE=${SPECFILE} \
    --tag "$DOCKER_TAG" \
    --file $DIST.Dockerfile \
    $(dirname $0)

echo "----------------------------------------------------"
echo "Running \"$COMMAND\" in docker image \"$DOCKER_TAG\""
mkdir -p rpmbuild/RPMS
docker run \
    --rm \
    --interactive \
    --tty \
    --mount type=bind,src=$PWD,dst=/work \
    --mount type=bind,src=$PWD/rpmbuild/RPMS,dst=/home/rpmbuild/rpmbuild/RPMS \
    --workdir /work \
    "$DOCKER_TAG" "$COMMAND" "$@"
