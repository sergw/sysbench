#!/usr/bin/env bash

#params: BRANCH, MY_EMAIL

if [ ! -n "${BRANCH}" ]; then BRANCH="1.8"; fi

curl -X POST -F token=386d57b7282d0fdf04f6461b83c073 -F ref=master \
    -F "variables[TIME]=100" \
    -F "variables[BRANCH]="${BRANCH} \
    -F "variables[MY_EMAIL]="${MY_EMAIL} \
    -F "variables[DCMAKE_BUILD_TYPE]=RelWithDebInfo" \
    https://gitlab.com/api/v4/projects/4186534/trigger/pipeline