#!/bin/sh

NAME=libsimplehttp
DIR=$NAME # rel to ..
YMD=`date '+%Y-%m-%d'`
TAR="${NAME}-${YMD}.tar"
TARGZ="${TAR}.gz"
cd ..
tar -cf $TAR $DIR  --exclude="$DIR/bak*"
gzip $TAR
file $TARGZ
ls -lt $TARGZ

#echo cp -purv $TARGZ ~/Development/scan/xplorer
#cp -purv $TARGZ ~/Development/scan/xplorer

