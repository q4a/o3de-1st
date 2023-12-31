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

#! ly_add_autogen: adds a code generation step to the specified target.
#
# \arg:NAME name of the target
# \arg:OUTPUT_NAME (optional) overrides the name of the output target. If not specified, the name will be used.
# \arg:INCLUDE_DIRECTORIES list of directories to use as include paths
# \arg:AUTOGEN_RULES a set of AutoGeneration rules to be passed to the AzAutoGen expansion system
# \arg:ALLFILES list of all source files contained by the target
function(ly_add_autogen)

    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs INCLUDE_DIRECTORIES AUTOGEN_RULES ALLFILES)
    cmake_parse_arguments(ly_add_autogen "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ly_add_autogen_AUTOGEN_RULES)
        set(AZCG_INPUTFILES ${ly_add_autogen_ALLFILES})
        list(FILTER AZCG_INPUTFILES INCLUDE REGEX ".*\.(xml|json|jinja)$")
        list(APPEND ly_add_autogen_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated")
        target_include_directories(${ly_add_autogen_NAME} ${ly_add_autogen_INCLUDE_DIRECTORIES})
        execute_process(
            COMMAND ${LY_PYTHON_CMD} "${CMAKE_SOURCE_DIR}/Code/Framework/AzAutoGen/AzAutoGen.py" "${CMAKE_BINARY_DIR}/Azcg/TemplateCache/" "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/" "${CMAKE_CURRENT_SOURCE_DIR}" "${AZCG_INPUTFILES}" "${ly_add_autogen_AUTOGEN_RULES}" "-n"
            OUTPUT_VARIABLE AUTOGEN_OUTPUTS
        )
        string(STRIP "${AUTOGEN_OUTPUTS}" AUTOGEN_OUTPUTS)
        set(AZCG_DEPENDENCIES ${AZCG_INPUTFILES})
        list(APPEND AZCG_DEPENDENCIES "${CMAKE_SOURCE_DIR}/Code/Framework/AzAutoGen/AzAutoGen.py")
        add_custom_command(
            OUTPUT ${AUTOGEN_OUTPUTS}
            DEPENDS ${AZCG_DEPENDENCIES}
            COMMAND ${CMAKE_COMMAND} -E echo "Running AutoGen for ${ly_add_autogen_NAME}"
            COMMAND ${LY_PYTHON_CMD} "${CMAKE_SOURCE_DIR}/Code/Framework/AzAutoGen/AzAutoGen.py" "${CMAKE_BINARY_DIR}/Azcg/TemplateCache/" "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/" "${CMAKE_CURRENT_SOURCE_DIR}" "${AZCG_INPUTFILES}" "${ly_add_autogen_AUTOGEN_RULES}"
            VERBATIM
        )
        set_target_properties(${ly_add_autogen_NAME} PROPERTIES AUTOGEN_INPUT_FILES "${AZCG_INPUTFILES}")
        set_target_properties(${ly_add_autogen_NAME} PROPERTIES AUTOGEN_OUTPUT_FILES "${AUTOGEN_OUTPUTS}")
        set_target_properties(${ly_add_autogen_NAME} PROPERTIES VS_USER_PROPS "${CMAKE_SOURCE_DIR}/Code/Framework/AzAutoGen/AzAutoGen.props")
        target_sources(${ly_add_autogen_NAME} PRIVATE ${AUTOGEN_OUTPUTS})
    endif()

endfunction()
