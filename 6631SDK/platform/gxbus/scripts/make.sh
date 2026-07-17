#!/bin/bash

DOMAKE="$(cd `dirname $0`; pwd)/platform/gxbus/scripts/domake.sh"

if [[ -e $DOMAKE ]];then
    bash $DOMAKE $* || { echo "【!!!compiled failed】【${BASH_SOURCE[0]}: $LINENO】Command: bash $DOMAKE $*"; exit 1; }
fi
