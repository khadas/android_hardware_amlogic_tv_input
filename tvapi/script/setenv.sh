#!/bin/bash
function usage () {
	echo "Usage:"
	echo "   run the script in tvapi top directory"
	echo "   source ./script/setenv.sh ----> default for ref board if not args"
    echo "   source ./script/setenv.sh ref_n300 -----> for ref n310    866 + 101"
	echo "   source ./script/setenv.sh skyworth_n310 -----> for skyworth n310    866 + 101"
}
CONFIG_VERSION="v1"
PWDPATH=$(pwd)

echo "PWD="
echo $PWDPATH

if [ $# -lt 1 ] || [ $1 = ref_n300 ]; then
    echo "for ref board"
    export TVAPI_TARGET_BOARD_VERSION="REF_N300"
    CONFIG_NAME="ref_n300"
    usage
 #   return
fi



if [ $# -ge 1 ]  && [ $1 = skyworth_n310 ] ; then
    echo "skyworth_n310---------------------"
    CONFIG_NAME=$1
    export TVAPI_TARGET_BOARD_VERSION="SKYWORTH_N310"
fi


CONFIG_FILE=$PWDPATH/build/include/xxxconfig.h
if [ -f  "$CONFIG_FILE" ]; then
rm $CONFIG_FILE
fi


PROJECT_PATH=${PWDPATH}/libtv/projects/${CONFIG_NAME}_${CONFIG_VERSION}
echo $PROJECT_PATH

TARGET_CONFIG_FILE="${CONFIG_NAME}_${CONFIG_VERSION}.h"

echo $CONFIG_NAME

if [ ! -f "${PROJECT_PATH}/${TARGET_CONFIG_FILE}" ]; then
    echo "TARGET_CONFIG_FILE=${TARGET_CONFIG_FILE} is not exist"
    return
fi



cat << EOF >> $CONFIG_FILE
#include <${CONFIG_NAME}_${CONFIG_VERSION}/$TARGET_CONFIG_FILE>
EOF
