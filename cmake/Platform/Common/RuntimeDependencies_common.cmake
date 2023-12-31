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

set(LY_COPY_PERMISSIONS "OWNER_READ OWNER_WRITE OWNER_EXECUTE")
set(LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE)

# There are several runtime dependencies to handle:
# 1. Dependencies to 3rdparty libraries. This involves copying IMPORTED_LOCATION to the folder where the target is. 
#    Some 3rdParty may require to copy the IMPORTED_LOCATION to a relative folder to where the target is.
# 2. Dependencies to files. This involves copying INTERFACE_LY_TARGET_FILES to the folder where the target is. In 
#    this case, the files may include a relative folder to where the target is.
# 3. In some platforms and types of targets, we also need to copy the MANUALLY_ADDED_DEPENDENCIES to the folder where the
#    target is. This is because the target is not in the same folder as the added dependencies. 
# In all cases, we need to recursively walk through dependencies to find the above. In multiple cases, we will end up 
# with the same files trying to be copied to the same place. This is expected, we still want to be able to produce a 
# working output per target, so there will be duplication.
#

function(ly_get_runtime_dependencies ly_RUNTIME_DEPENDENCIES ly_TARGET)
    # check to see if this target is a 3rdParty lib that was a downloaded package,
    # and if so, activate it.  This also calls find_package so there is no reason
    # to do so later.

    ly_parse_third_party_dependencies(${ly_TARGET})
    # The above needs to be done before the below early out of this function!
    if(NOT TARGET ${ly_TARGET})
        return() # Nothing to do
    endif()

    # To optimize the search, we are going to cache the dependencies for the targets we already walked through.
    # To do so, we will create a variable named LY_RUNTIME_DEPENDENCIES_${ly_TARGET} which will contain a list
    # of all the dependencies
    # If the variable is not there, that means we have not walked it yet.
    get_property(are_dependencies_cached GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} SET)
    if(are_dependencies_cached)

        # We already walked through this target
        get_property(cached_dependencies GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET})
        set(${ly_RUNTIME_DEPENDENCIES} ${cached_dependencies} PARENT_SCOPE)
        return()

    endif()

    unset(all_runtime_dependencies)
    
    # Collect all dependencies to other targets. Dependencies are through linking (LINK_LIBRARIES), and
    # other manual dependencies (MANUALLY_ADDED_DEPENDENCIES)

    get_target_property(target_type ${ly_TARGET} TYPE)
    unset(link_dependencies)
    unset(dependencies)
    get_target_property(dependencies ${ly_TARGET} INTERFACE_LINK_LIBRARIES)
    if(dependencies)
        list(APPEND link_dependencies ${dependencies})
    endif()
    if(NOT target_type MATCHES "INTERFACE")
        unset(dependencies)
        get_target_property(dependencies ${ly_TARGET} LINK_LIBRARIES)
        if(dependencies)
            list(APPEND link_dependencies ${dependencies})
        endif()
    endif()

    # link dependencies are not runtime dependencies (we dont have anything to copy) however, we need to traverse
    # them since them or some dependency downstream could have something to copy over
    foreach(link_dependency ${link_dependencies})
        if(NOT ${link_dependency} MATCHES "^::@") # Skip wraping produced when targets are not created in the same directory (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)
            unset(dependencies)
            ly_get_runtime_dependencies(dependencies ${link_dependency})
            list(APPEND all_runtime_dependencies ${dependencies})
        endif()
    endforeach()

    # For manual dependencies, we want to copy over the dependency and traverse them
    unset(manual_dependencies)
    get_target_property(manual_dependencies ${ly_TARGET} MANUALLY_ADDED_DEPENDENCIES)
    if(manual_dependencies)
        foreach(manual_dependency ${manual_dependencies})
            if(NOT ${manual_dependency} MATCHES "^::@") # Skip wraping produced when targets are not created in the same directory (https://cmake.org/cmake/help/latest/prop_tgt/LINK_LIBRARIES.html)
                unset(dependencies)
                ly_get_runtime_dependencies(dependencies ${manual_dependency})
                list(APPEND all_runtime_dependencies ${dependencies})
                list(APPEND all_runtime_dependencies ${manual_dependency})
            endif()
        endforeach()
    endif()

    # Add the imported locations
    get_target_property(is_imported ${ly_TARGET} IMPORTED)
    if(is_imported)
        # Skip Qt if this is a 3rdParty
        # Qt is deployed using qt_deploy, no need to copy the dependencies
        set(skip_imported FALSE)
        string(REGEX MATCH "3rdParty::([^:,]*)" target_package ${ly_TARGET})
        if(target_package)
            if(${CMAKE_MATCH_1} STREQUAL "Qt")
                set(skip_imported TRUE)
            endif()
        endif()

        if(NOT skip_imported)

            # Add imported locations
            if(target_type MATCHES "INTERFACE")
                set(imported_property INTERFACE_IMPORTED_LOCATION)
            else()
                set(imported_property IMPORTED_LOCATION)
            endif()
            get_target_property(target_locations ${ly_TARGET} ${imported_property})

            if(target_locations)
                list(APPEND all_runtime_dependencies ${target_locations})
            endif()

        endif()

    endif()

    # Add target files (these are the ones added with ly_add_target_files)
    get_target_property(interface_target_files ${ly_TARGET} INTERFACE_LY_TARGET_FILES)
    if(interface_target_files)
        list(APPEND all_runtime_dependencies ${interface_target_files})
    endif()

    list(REMOVE_DUPLICATES all_runtime_dependencies)
    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCIES_${ly_TARGET} "${all_runtime_dependencies}")
    set(${ly_RUNTIME_DEPENDENCIES} ${all_runtime_dependencies} PARENT_SCOPE)

endfunction()

function(ly_get_runtime_dependency_command ly_RUNTIME_COMMAND ly_TARGET)

    # To optimize this, we are going to cache the commands for the targets we requested. A lot of targets end up being
    # dependencies of other targets. 
    get_property(is_command_cached GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET} SET)
    if(is_command_cached)

        # We already walked through this target
        get_property(cached_command GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET})
        set(${ly_RUNTIME_COMMAND} ${cached_command} PARENT_SCOPE)
        return()

    endif()

    unset(target_directory)
    unset(source_file)
    if(TARGET ${ly_TARGET})

        get_target_property(target_type ${ly_TARGET} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            return()
        endif()

        set(source_file $<TARGET_FILE:${ly_TARGET}>)
        get_target_property(runtime_directory ${ly_TARGET} RUNTIME_OUTPUT_DIRECTORY)
        if(runtime_directory)
            file(RELATIVE_PATH target_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${runtime_directory})
        endif()

    else()

        string(REGEX MATCH "^([^\n]*)[\n]?(.*)$" target_file_regex "${ly_TARGET}")
        if(NOT target_file_regex)
            message(FATAL_ERROR "Unexpected error parsing \"${ly_TARGET}\"")
        endif()
        set(source_file ${CMAKE_MATCH_1})
        set(target_directory ${CMAKE_MATCH_2})

    endif()
    if(target_directory)
        string(PREPEND target_directory /)
    endif()

    # Some notes on the generated command:
    # To support platforms where the binaries end in different places, we are going to assume that all dependencies,
    # including the ones we are building, need to be copied over. However, we add a check to prevent copying something
    # over itself. This detection cannot happen now because the target we are copying for varies.
    set(runtime_command "ly_copy(\"${source_file}\" \"$<TARGET_FILE_DIR:@target@>${target_directory}\")\n")

    # Tentative optimization: this is an attempt to solve the first "if" at generation time, making the runtime_dependencies
    # file smaller and faster to run. In platforms where the built target and the dependencies targets end up in the same
    # place, we end up with a lot of dependencies that dont need to copy anything.
    # However, the generation expression ends up being complicated and the parser seems to get really confused. My hunch
    # is that the string we are pasting has generation expressions, and those commas confuse the "if" generator expression.
    # Leaving the attempt here commented out until I can get back to it.
#    set(generated_command "
#if(NOT EXISTS \"$<TARGET_FILE_DIR:@target@>${target_directory}\")
#    file(MAKE_DIRECTORY \"$<TARGET_FILE_DIR:@target@>${target_directory}\")
#endif()
#if(\"${source_file}\" IS_NEWER_THAN \"$<TARGET_FILE_DIR:@target@>${target_directory}/${target_filename}\")
#    file(COPY \"${source_file}\" DESTINATION \"$<TARGET_FILE_DIR:@target@>${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS})
#endif()
#")
#    set(runtime_command "$<IF:$<STREQUAL:$<GENEX_EVAL:\"${source_file}\">,$<GENEX_EVAL:\"$<TARGET_FILE_DIR:@target@>${target_directory}/${target_filename}>\">>,\"\",\"$<GENEX_EVAL:${generated_command}>\">")

    set_property(GLOBAL PROPERTY LY_RUNTIME_DEPENDENCY_COMMAND_${ly_TARGET} "${runtime_command}")
    set(${ly_RUNTIME_COMMAND} ${runtime_command} PARENT_SCOPE)

endfunction()

get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
list(APPEND CMAKE_MODULE_PATH ${additional_module_paths})

get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
foreach(target IN LISTS all_targets)

    # Exclude targets that dont produce runtime outputs
    get_target_property(target_type ${target} TYPE)
    if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
        continue()
    endif()

    unset(runtime_dependencies)
    set(runtime_commands "
function(ly_copy source_file target_directory)
    get_filename_component(target_filename \"\${source_file}\" NAME)
    if(NOT \"\${source_file}\" STREQUAL \"\${target_directory}/\${target_filename}\")
        if(NOT EXISTS \"\${target_directory}\")
            file(MAKE_DIRECTORY \"\${target_directory}\")
        endif()
        if(\"\${source_file}\" IS_NEWER_THAN \"\${target_directory}/\${target_filename}\")
            file(LOCK \"\${target_directory}/\${target_filename}.lock\" GUARD FUNCTION TIMEOUT 10)
            file(COPY \"\${source_file}\" DESTINATION \"\${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS})
        endif()
    endif()    
endfunction()
\n")

    ly_get_runtime_dependencies(runtime_dependencies ${target})
    foreach(runtime_dependency ${runtime_dependencies})
        unset(runtime_command)
        ly_get_runtime_dependency_command(runtime_command ${runtime_dependency})
        string(APPEND runtime_commands ${runtime_command})
    endforeach()
    
    # Generate the output file
    string(CONFIGURE "${runtime_commands}" generated_commands @ONLY)
    file(GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${target}.cmake
        CONTENT "${generated_commands}"
    )

endforeach()

