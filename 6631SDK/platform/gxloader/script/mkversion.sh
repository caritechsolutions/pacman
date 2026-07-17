#!/bin/bash

TIMESTAMP=`env LC_TIME=en_US.UTF-8 date`
if [ -d ".git" ]
then
LOCAL_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ ${LOCAL_BRANCH} = HEAD ]
then
LOADER_BRANCH=$(git remote | awk -F '/' '{ print $NF}')
else
LOADER_BRANCH=$(git config --list | grep ${LOCAL_BRANCH}.merge | awk -F '=' '{ print $NF}' | awk -F '/' '{ print $NF}')
fi
GITCOMMITID=$(git log -1 | grep -m1 "commit" | awk -F ' ' '{ print $NF}')
fi
LOADER_BANNER="${LOADER_BRANCH} (${GITCOMMITID}) ${TIMESTAMP}"
echo $LOADER_BANNER

