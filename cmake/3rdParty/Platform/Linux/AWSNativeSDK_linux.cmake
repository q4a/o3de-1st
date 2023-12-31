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

if(LY_MONOLITHIC_GAME)
    set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/lib/linux/$<IF:$<CONFIG:Debug>,Debug,Release>)
    set(AWSNATIVE_SDK_LIB_EXTENSION a)
else()
    set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/bin/linux/$<IF:$<CONFIG:Debug>,Debug,Release>)
    set(AWSNATIVE_SDK_LIB_EXTENSION so)
    set(AWSNATIVESDK_COMMON_UNSTABLE_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-common.so.0unstable)
    set(AWSNATIVESDK_EVENTSTREAM_UNSTABLE_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-event-stream.so.0unstable)
endif()

set(AWSNATIVESDK_LIBS
    curl # The one bundled with the aws sdk in 3rdParty doesn't use the right openssl
)

set(AWSNATIVESDK_CORE_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-core.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_COMMON_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-common.${AWSNATIVE_SDK_LIB_EXTENSION} ${AWSNATIVESDK_COMMON_UNSTABLE_LIBS})
set(AWSNATIVESDK_EVENTSTREAM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-event-stream.${AWSNATIVE_SDK_LIB_EXTENSION} ${AWSNATIVESDK_EVENTSTREAM_UNSTABLE_LIBS})
set(AWSNATIVESDK_CHECKSUMS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-checksums.${AWSNATIVE_SDK_LIB_EXTENSION})

set(AWSNATIVESDK_ACESSMANAGEMENT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-access-management.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_COGNITOIDENTITY_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-cognito-identity.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_COGNITOIDP_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-cognito-idp.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_DEVICEFARM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-devicefarm.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_DYNAMODB_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-dynamodb.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_GAMELIFT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-gamelift.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_IDENTITYMANAGEMENT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-identity-management.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_KINESIS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-kinesis.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_LAMBDA_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-lambda.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_MOBILEANALYTICS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-mobileanalytics.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_QUEUES_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-queues.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_S3_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-s3.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_SNS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sns.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_SQS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sqs.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_STS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sts.${AWSNATIVE_SDK_LIB_EXTENSION})
set(AWSNATIVESDK_TRANSFER_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-transfer.${AWSNATIVE_SDK_LIB_EXTENSION})

if(NOT LY_MONOLITHIC_GAME)

    list(JOIN AWSNATIVESDK_CORE_LIBS ";" AWSNATIVESDK_CORE_RUNTIME_DEPENDENCIES)

    set(AWSNATIVESDK_COMMON_RUNTIME_DEPENDENCIES                ${AWSNATIVESDK_COMMON_LIBS})
    set(AWSNATIVESDK_EVENTSTREAM_RUNTIME_DEPENDENCIES           ${AWSNATIVESDK_EVENTSTREAM_LIBS})
    set(AWSNATIVESDK_CHECKSUMS_RUNTIME_DEPENDENCIES             ${AWSNATIVESDK_CHECKSUMS_LIBS})
    set(AWSNATIVESDK_ACCESSMANAGEMENT_RUNTIME_DEPENDENCIES      ${AWSNATIVESDK_ACESSMANAGEMENT_LIBS})
    set(AWSNATIVESDK_COGNITOIDENTITY_RUNTIME_DEPENDENCIES       ${AWSNATIVESDK_COGNITOIDENTITY_LIBS})
    set(AWSNATIVESDK_COGNITOIDP_RUNTIME_DEPENDENCIES            ${AWSNATIVESDK_COGNITOIDP_LIBS})
    set(AWSNATIVESDK_DEVICEFARM_RUNTIME_DEPENDENCIES            ${AWSNATIVESDK_DEVICEFARM_LIBS})
    set(AWSNATIVESDK_DYNAMODB_RUNTIME_DEPENDENCIES              ${AWSNATIVESDK_DYNAMODB_LIBS})
    set(AWSNATIVESDK_GAMELIFT_RUNTIME_DEPENDENCIES              ${AWSNATIVESDK_GAMELIFT_LIBS})
    set(AWSNATIVESDK_IDENTITYMANAGEMENT_RUNTIME_DEPENDENCIES    ${AWSNATIVESDK_IDENTITYMANAGEMENT_LIBS})
    set(AWSNATIVESDK_KINESIS_RUNTIME_DEPENDENCIES               ${AWSNATIVESDK_KINESIS_LIBS})
    set(AWSNATIVESDK_LAMBDA_RUNTIME_DEPENDENCIES                ${AWSNATIVESDK_LAMBDA_LIBS})
    set(AWSNATIVESDK_MOBILEANALYTICS_RUNTIME_DEPENDENCIES       ${AWSNATIVESDK_MOBILEANALYTICS_LIBS})
    set(AWSNATIVESDK_QUEUES_RUNTIME_DEPENDENCIES                ${AWSNATIVESDK_QUEUES_LIBS})

    set(AWSNATIVESDK_S3_RUNTIME_DEPENDENCIES                    ${AWSNATIVESDK_S3_LIBS})
    set(AWSNATIVESDK_SNS_RUNTIME_DEPENDENCIES                   ${AWSNATIVESDK_SNS_LIBS})
    set(AWSNATIVESDK_SQS_RUNTIME_DEPENDENCIES                   ${AWSNATIVESDK_SQS_LIBS})
    set(AWSNATIVESDK_STS_RUNTIME_DEPENDENCIES                   ${AWSNATIVESDK_STS_LIBS})
    set(AWSNATIVESDK_TRANSFER_RUNTIME_DEPENDENCIES              ${AWSNATIVESDK_TRANSFER_LIBS})

endif()
