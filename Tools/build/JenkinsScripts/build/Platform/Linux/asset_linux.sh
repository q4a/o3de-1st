#!/usr/bin/env bash
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set -o errexit # exit on the first failure encountered

# Delete output directory if CLEAN_OUTPUT_DIRECTORY env variable is set
if [[ $CLEAN_OUTPUT_DIRECTORY == "true" ]]; then
    if [[ -d Cache ]]; then
        echo "[ci_build] CLEAN_OUTPUT_DIRECTORY option set with value \"${CLEAN_OUTPUT_DIRECTORY}\""
        echo "[ci_build] Deleting \"Cache\""
        rm -rf Cache
    fi
fi

if [[ ! -d $OUTPUT_DIRECTORY ]]; then
    echo [ci_build] Error: $OUTPUT_DIRECTORY was not found
    exit 1
fi
pushd $OUTPUT_DIRECTORY

if [[ ! -e $ASSET_PROCESSOR_BINARY ]]; then
    echo [ci_build] Error: $ASSET_PROCESSOR_BINARY was not found
    exit 1
fi

for project in $(echo $CMAKE_LY_PROJECTS | sed "s/;/ /g")
do
    echo [ci_build] ${ASSET_PROCESSOR_BINARY} $ASSET_PROCESSOR_OPTIONS --gamefolder=$project --platforms=$ASSET_PROCESSOR_PLATFORMS
    ${ASSET_PROCESSOR_BINARY} $ASSET_PROCESSOR_OPTIONS --gamefolder=$project --platforms=$ASSET_PROCESSOR_PLATFORMS
done

popd
