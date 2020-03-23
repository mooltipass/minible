#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

VERSION="$(get_version .)"

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(git rev-parse HEAD)"  ]; then
    echo "Not uploading package"
    exit 0
fi

# Debian package
docker exec minible-deb bash /scripts/build_source.sh $VERSION xenial
docker exec minible-deb bash /scripts/build_source.sh $VERSION bionic
docker exec minible-deb bash /scripts/build_source.sh $VERSION disco
docker exec minible-deb bash /scripts/build_source.sh $VERSION eoan

#prepare files to upload volume
mkdir -p $HOME/uploads
cp -v \
    output/*.{exe,zip} \
    $HOME/uploads/

EXE_FILE="$(ls $HOME/uploads/*.exe 2> /dev/null | head -n 1)"

#docker run -t --name mc-upload -d \
#    -v $HOME/uploads:/uploads \
#	-e "GITHUB_LOGIN=${GITHUB_LOGIN}" \
#	-e "GITHUB_TOKEN=${GITHUB_TOKEN}" \
#    mooltipass/mc-upload

