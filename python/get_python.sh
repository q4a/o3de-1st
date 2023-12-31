#!/bin/bash

# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [ -h "$SOURCE" ]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done

DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
cd $DIR

# Overall strategy: Run python with --version to see if its already there
# otherwise, use cmake and the new package system to download it.
# To find cmake, search the path for cmake.  If not found, try a fixed known location.

# the version number below is only used if cmake isn't already on your path.
# if you update this version number, remember to update the one(s) in the other platform
# files, as well as in Tools/build/Jenkins/...

./python.sh --version > /dev/null
python_exitcode=$?
if [ $python_exitcode == 0 ]; then
    echo get_python.sh: Python is already downloaded: $(./python.sh --version)
    $DIR/pip.sh install -r $DIR/requirements.txt --quiet --disable-pip-version-check
    exit 0
fi
if [[ "$OSTYPE" = *"darwin"* ]];
then
    PAL=Mac
    CMAKE_FOLDER_RELATIVE_TO_ROOT=CMake.app/Contents/bin
else
    PAL=Linux
    CMAKE_FOLDER_RELATIVE_TO_ROOT=bin    
fi

if ! [ -x "$(command -v cmake)" ]; then
    # Note that LY_3RDPARTY_PATH is only required here if you have no cmake in your PATH.
    if [ -z ${LY_CMAKE_PATH} ]; then
        if [ -z ${LY_3RDPARTY_PATH} ]; then 
            echo "ERROR: Could not find cmake on the PATH and LY_3RDPARTY_PATH is not defined, cannot continue."
            echo "Please add cmake to your PATH, or define $LY_3RDPARTY_PATH"
            exit 1        
        fi
        LY_CMAKE_PATH=$LY_3RDPARTY_PATH/CMake/3.19.1/$PAL/$CMAKE_FOLDER_RELATIVE_TO_ROOT
        # if you change the version number, change it also in:
        # Tools/build/JenkinsScripts/build/Platform/Mac/env_mac.sh
        # and
        # Tools/build/JenkinsScripts/build/Platform/Linux/env_linux.sh
    fi
        
    export PATH=$LY_CMAKE_PATH:$PATH
    if ! [ -x "$(command -v cmake)" ]; then
        echo "ERROR: Could not find cmake on the PATH or at the known location: $CMAKE_KNOWN_LOCATION"
        echo "Please add cmake to the environment PATH or place it at the above known location."
        exit 1
    else
        echo "CMake not found on path, but was found in the known 3rd Party location."
    fi
fi

echo Using cmake located at: $(which cmake)
echo $(cmake --version)

cd ..

cmake -DPAL_PLATFORM_NAME:string=$PAL -DLY_3RDPARTY_PATH:string=$DIR  -P $DIR/get_python.cmake

retVal=$?
if [ $retVal -ne 0 ]; then
    echo Unable to fetch python using cmake.  
    echo  - Is LY_PACKAGE_SERVER_URLS set?  
    echo  - Do you have permission to access the packages?
    exit $retVal
fi

echo installing via pip...
$DIR/pip.sh install -r $DIR/requirements.txt --disable-pip-version-check
exit $?
