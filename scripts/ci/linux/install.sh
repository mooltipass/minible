#!/bin/bash
set -ev

#docker run -t --name winbuilder -d \
#    -v $(pwd):/moolticute \
#    -v $HOME/cert.p12:/cert.p12 \
#    mooltipass/mc-win-builder

docker run -t --name minible-deb -d \
    -v $HOME/build-debs:/minible \
    mooltipass/minible-launchpad

docker ps
