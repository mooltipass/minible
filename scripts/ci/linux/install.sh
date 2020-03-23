#!/bin/bash
set -ev

docker run -t --name miniblebuilder -d \
    -v $(pwd):/minible \
    mooltipass/minible-win-builder

docker run -t --name minible-deb -d \
    -v $HOME/build-debs:/minible \
    mooltipass/minible-launchpad

docker ps
