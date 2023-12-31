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

set(LY_ROOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

set(LY_UNITY_BUILD OFF CACHE BOOL "UNITY builds")

include(CMakeFindDependencyMacro)
include(CMakeParseArguments)
include(cmake/LyAutoGen.cmake)

ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})
include(${pal_dir}/LYWrappers_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Not all platforms support unity builds
if(LY_UNITY_BUILD AND NOT PAL_TRAIT_BUILD_UNITY_SUPPORTED)
    message(ERROR "LY_UNITY_BUILD is specified, but not supported for the current target platform")
endif()


#! ly_add_target: adds a target and provides parameters for the common configurations.
#
# Adds a target (static/dynamic library, executable) and convenient wrappers around most
# common parameters that need to be set.
# This function also creates an interface to use for dependencies. The interface will be 
# named as "NAMESPACE::NAME"
# Some examples:
#     ly_add_target(NAME mystaticlib STATIC FILES_CMAKE somestatic_files.cmake)
#     ly_add_target(NAME mydynamiclib SHARED FILES_CMAKE somedyn_files.cmake)
#     ly_add_target(NAME myexecutable EXECUTABLE FILES_CMAKE someexe_files.cmake)
#
# \arg:NAME name of the target
# \arg:STATIC (bool) defines this target to be a static library
# \arg:SHARED (bool) defines this target to be a dynamic library
# \arg:MODULE (bool) defines this target to be a module library
# \arg:HEADERONLY (bool) defines this target to be a header only library. A ${NAME}_HEADERS project will be created for the IDE
# \arg:EXECUTABLE (bool) defines this target to be an executable
# \arg:APPLICATION (bool) defines this target to be an application (executable that is not a console)
# \arg:NAMESPACE namespace declaration for this target. It will be used for IDE and dependencies
# \arg:OUTPUT_NAME (optional) overrides the name of the output target. If not specified, the name will be used.
# \arg:OUTPUT_SUBDIRECTORY places the runtime binary in a subfolder within the output folder (this only affects to runtime binaries)
# \arg:AUTOMOC enables Qt moc in the target
# \arg:AUTOUIC enables Qt uic in the target
# \arg:AUTORCC enables Qt rcc in the target
# \arg:NO_UNITY Prevent the target from employing unity builds even when unity builds (LY_UNITY_BUILD) are enabled
# \arg:FILES_CMAKE list of *_files.cmake files that contain files for this target
# \arg:GENERATED_FILES list of files to add to this target that are generated out of some other task
# \arg:INCLUDE_DIRECTORIES list of directories to use as include paths
# \arg:BUILD_DEPENDENCIES list of interfaces this target depends on (could be a compilation dependency
#                             if the dependency is only exposing an include path, or could be a linking
#                             dependency is exposing a lib)
# \arg:RUNTIME_DEPENDENCIES list of dependencies this target depends on at runtime
# \arg:COMPILE_DEFINITIONS list of compilation definitions this target will use to compile
# \arg:PLATFORM_INCLUDE_FILES *.cmake files which should contain platform specific configuration to be added to the target
#                             Look at the documentation for the ly_configure_target_platform_properties() function below
#                             for the list of variables that will be used by the target
# \arg:TARGET_PROPERTIES additional properties to set to the target
# \arg:AUTOGEN_RULES a set of AutoGeneration rules to be passed to the AzAutoGen expansion system
function(ly_add_target)

    set(options STATIC SHARED MODULE HEADERONLY EXECUTABLE APPLICATION AUTOMOC AUTOUIC AUTORCC NO_UNITY)
    set(oneValueArgs NAME NAMESPACE OUTPUT_SUBDIRECTORY OUTPUT_NAME)
    set(multiValueArgs FILES_CMAKE GENERATED_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS BUILD_DEPENDENCIES RUNTIME_DEPENDENCIES PLATFORM_INCLUDE_FILES TARGET_PROPERTIES AUTOGEN_RULES)

    cmake_parse_arguments(ly_add_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Validate input arguments
    if(NOT ly_add_target_NAME)
        message(FATAL_ERROR "You must provide a name for the target")
    endif()
    if(NOT ly_add_target_FILES_CMAKE)
        message(FATAL_ERROR "You must provide a list of _files.cmake files for the target")
    endif()

    foreach(file_cmake ${ly_add_target_FILES_CMAKE})
        ly_include_cmake_file_list(${file_cmake})
    endforeach()

    set(linking_options)
    set(linking_count)
    if(ly_add_target_STATIC)
        set(linking_options STATIC)
        set(linking_count "${linking_count}1")
    endif()
    if(ly_add_target_SHARED)
        set(linking_options SHARED)
        set(linking_count "${linking_count}1")
    endif()
    if(ly_add_target_MODULE)
        set(linking_options ${PAL_LINKOPTION_MODULE})
        set(linking_count "${linking_count}1")
    endif()

    if(ly_add_target_HEADERONLY)
        set(linking_options INTERFACE)
        set(linking_count "${linking_count}1")
    endif()
    if(ly_add_target_EXECUTABLE)
        set(linking_options EXECUTABLE)
        set(linking_count "${linking_count}1")
    endif()
    if(ly_add_target_APPLICATION)
        set(linking_options APPLICATION)
        set(linking_count "${linking_count}1")
    endif()
    if(NOT ("${linking_count}" STREQUAL "1"))
        message(FATAL_ERROR "More than one of the following options [STATIC | SHARED | MODULE | HEADERONLY | EXECUTABLE | APPLICATION] was specified and they are mutually exclusive")
    endif()

    if(ly_add_target_NAMESPACE)
        set(interface_name "${ly_add_target_NAMESPACE}::${ly_add_target_NAME}")
    else()
        set(interface_name "${ly_add_target_NAME}")
    endif()

    set(project_NAME ${ly_add_target_NAME})
    if(ly_add_target_EXECUTABLE)
        add_executable(${ly_add_target_NAME}
            ${ALLFILES} ${ly_add_target_GENERATED_FILES}
        )
        ly_apply_platform_properties(${ly_add_target_NAME})
    elseif(ly_add_target_APPLICATION)
        add_executable(${ly_add_target_NAME}
            ${PAL_EXECUTABLE_APPLICATION_FLAG}
            ${ALLFILES} ${ly_add_target_GENERATED_FILES}
        )
        ly_apply_platform_properties(${ly_add_target_NAME})
    elseif(ly_add_target_HEADERONLY)
        add_library(${ly_add_target_NAME}
            ${linking_options}
        )
        # Create a custom target to see it in the IDE
        # Once this is solved: https://gitlab.kitware.com/cmake/cmake/merge_requests/3113, we can just add the files to the library above
        add_custom_target(${ly_add_target_NAME}_HEADERS
            SOURCES ${ALLFILES} ${ly_add_target_GENERATED_FILES}
        )
        set(project_NAME ${ly_add_target_NAME}_HEADERS)
    else()
        add_library(${ly_add_target_NAME}
            ${linking_options}
            ${ALLFILES} ${ly_add_target_GENERATED_FILES}
        )
        ly_apply_platform_properties(${ly_add_target_NAME})
    endif()
    if(${ly_add_target_GENERATED_FILES})
        set_source_files_properties(${ly_add_target_GENERATED_FILES}
            PROPERTIES GENERATED TRUE
        )
    endif()

    if(${ly_add_target_EXECUTABLE} OR ${ly_add_target_APPLICATION})
        add_executable(${interface_name} ALIAS ${ly_add_target_NAME})
    else()
        add_library(${interface_name} ALIAS ${ly_add_target_NAME})
    endif()

    if(ly_add_target_OUTPUT_NAME)
        set_target_properties(${ly_add_target_NAME} PROPERTIES 
            OUTPUT_NAME ${ly_add_target_OUTPUT_NAME}
        )
    endif()

    if (ly_add_target_SHARED OR ly_add_target_MODULE OR ly_add_target_EXECUTABLE OR ly_add_target_APPLICATION)

        if (ly_add_target_OUTPUT_SUBDIRECTORY)
            ly_handle_custom_output_directory(${ly_add_target_NAME} ${ly_add_target_OUTPUT_SUBDIRECTORY})
        else()
            ly_handle_custom_output_directory(${ly_add_target_NAME} "")
        endif()

    endif()

    if (ly_add_target_INCLUDE_DIRECTORIES)
        target_include_directories(${ly_add_target_NAME}
            ${ly_add_target_INCLUDE_DIRECTORIES}
        )
    endif()

    # Parse the 3rdParty library dependencies 
    ly_parse_third_party_dependencies("${ly_add_target_BUILD_DEPENDENCIES}")
    ly_target_link_libraries(${ly_add_target_NAME}
        ${ly_add_target_BUILD_DEPENDENCIES}
    )

    if(ly_add_target_COMPILE_DEFINITIONS)
        target_compile_definitions(${ly_add_target_NAME}
            ${ly_add_target_COMPILE_DEFINITIONS}
        )
    endif()

    # For any target that depends on AzTest and is built as an executable, an additional 'AZ_TEST_EXECUTABLE' define will
    # enable the 'AZ_UNIT_TEST_HOOK' macro to also implement main() so that running the executable directly will run
    # the AZ_UNIT_TEST_HOOK function
    if (${PAL_TRAIT_TEST_TARGET_TYPE} STREQUAL "EXECUTABLE" AND "AZ::AzTest" IN_LIST ly_add_target_BUILD_DEPENDENCIES)
        target_compile_definitions(${ly_add_target_NAME}
            PRIVATE
                AZ_TEST_EXECUTABLE
        )
    endif()

    if(ly_add_target_TARGET_PROPERTIES)
        set_target_properties(${ly_add_target_NAME} PROPERTIES
            ${ly_add_target_TARGET_PROPERTIES})
    endif()

    set(unity_target_types SHARED MODULE EXECUTABLE APPLICATION STATIC)
    if(linking_options IN_LIST unity_target_types)
        # For eligible target types, if unity builds (LY_UNITY_BUILD) is enabled and the target is not marked with 'NO_UNITY',
        # enable UNITY builds individually for the target
        if(LY_UNITY_BUILD AND NOT ${ly_add_target_NO_UNITY})
            set_target_properties(${ly_add_target_NAME} PROPERTIES
                UNITY_BUILD ON
                UNITY_BUILD_MODE BATCH
            )
        endif()
    endif()
    
    # IDE organization
    ly_source_groups_from_folders("${ALLFILES}")
    source_group("Generated Files" REGULAR_EXPRESSION "(${CMAKE_BINARY_DIR})") # Any file coming from the output folder
    file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR})
    set_property(TARGET ${project_NAME} PROPERTY FOLDER ${project_path})

    if(ly_add_target_RUNTIME_DEPENDENCIES)
        ly_parse_third_party_dependencies("${ly_add_target_RUNTIME_DEPENDENCIES}")
        ly_add_dependencies(${ly_add_target_NAME} ${ly_add_target_RUNTIME_DEPENDENCIES})
    endif()

    ly_configure_target_platform_properties()

    # Handle Qt MOC, RCC, UIC
    # https://gitlab.kitware.com/cmake/cmake/issues/18749
    # AUTOMOC is supposed to always rebuild because it checks files that are not listed in the sources (like extra 
    # "_p.h" headers) and which may change outside the visibility of the generator. 
    # We are not using AUTOUIC because of:
    # https://gitlab.kitware.com/cmake/cmake/-/issues/18741
    # To overcome this problem, we manually wrap all the ui files listed in the target with qt5_wrap_ui
    foreach(prop IN ITEMS AUTOMOC AUTORCC)
        if(${ly_add_target_${prop}})
            set_property(TARGET ${ly_add_target_NAME} PROPERTY ${prop} ON)
            # Flag this target as depending on Qt
            set_property(GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${ly_add_target_NAME} ON)
        endif()
    endforeach()
    if(${ly_add_target_AUTOUIC})
        get_target_property(all_ui_sources ${ly_add_target_NAME} SOURCES)
        list(FILTER all_ui_sources INCLUDE REGEX "^.*\\.ui$")
        if(NOT all_ui_sources)
            message(FATAL_ERROR "Target ${ly_add_target_NAME} contains AUTOUIC but doesnt have any .ui file")
        endif()
        ly_qt_uic_target(${ly_add_target_NAME})
    endif()

    # Add dependencies that were added before this target was available
    get_property(additional_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${ly_add_target_NAME})
    if(additional_dependencies)
        ly_add_dependencies(${ly_add_target_NAME} ${additional_dependencies})
        # Clear the variable so we can track issues in case some dependency is added after
        set_property(GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_${ly_add_target_NAME})
    endif()

    # Store the target so we can walk through all of them in LocationDependencies.cmake
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TARGETS ${ly_add_target_NAME})

    set(runtime_dependencies_list SHARED MODULE EXECUTABLE APPLICATION)
    if(linking_options IN_LIST runtime_dependencies_list)

        add_custom_command(TARGET ${ly_add_target_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/runtime_dependencies/$<CONFIG>/${ly_add_target_NAME}.cmake
            DEPENDS ${CMAKE_BINARY_DIR}/runtime_dependencies/${ly_add_target_NAME}.cmake
            MESSAGE "Copying runtime dependencies..."
            VERBATIM
        )

        detect_qt_dependency(${ly_add_target_NAME} QT_DEPENDENCY)
        if(QT_DEPENDENCY)
            if(NOT COMMAND ly_qt_deploy)
                message(FATAL_ERROR "Could not find function \"ly_qt_deploy\", this function should be defined in cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/Qt_${PAL_PLATFORM_NAME_LOWERCASE}.cmake")
            endif()

            ly_qt_deploy(TARGET ${ly_add_target_NAME})
        endif()
    endif()

    if(ly_add_target_AUTOGEN_RULES)
        ly_add_autogen(
            NAME ${ly_add_target_NAME}
            INCLUDE_DIRECTORIES ${ly_add_target_INCLUDE_DIRECTORIES}
            AUTOGEN_RULES ${ly_add_target_AUTOGEN_RULES} 
            ALLFILES ${ALLFILES}
        )
    endif()

endfunction()

#! ly_target_link_libraries: wraps target_link_libraries handling also MODULE linkage.
#  MODULE libraries cannot be passed to target_link_libraries. MODULE libraries are shared libraries that we 
#  dont want to link against because they will be loaded dynamically. However, we want to include their public headers
#  and transition their public dependencies. 
#  To achieve this, we delay the target_link_libraries call to after all targets are declared (see ly_delayed_target_link_libraries)
#
#  Signature is the same as target_link_libraries:
#     target_link_libraries(<target> ... <item>... ...)
#
function(ly_target_link_libraries TARGET)

    if(NOT TARGET)
        message(FATAL_ERROR "You must provide a target")
    endif()
    
    set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LINK_${TARGET} ${ARGN})
    set_property(GLOBAL APPEND PROPERTY LY_DELAYED_LINK_TARGETS ${TARGET}) # to walk them at the end

endfunction()

#! ly_delayed_target_link_libraries: internal function called by the root CMakeLists.txt after all targets
#  have been declared to determine if they are regularly 
#  1) If a MODULE is passed in the list of items, it will add the INTERFACE_INCLUDE_DIRECTORIES as include
#     directories of TARGET. It will also add the "INTERFACE_LINK_LIBRARIES" to TARGET. MODULEs cannot be
#     directly linked, but we can include the public headers and link against the things the MODULE expose
#     to link.
#  2) If a target that has not yet been declared is passed, then it will defer it to after all targets are
#     declared. This way we can do a check again. We could delay the link to when 
#     target is declared. This is needed for (1) since we dont know the type of target. This also addresses
#     another issue with target_link_libraries where it will only validate that a MODULE is not being passed
#     if the target is already declared, if not, it will fail later at linking time.

function(ly_delayed_target_link_libraries)
    
    set(visibilities PRIVATE PUBLIC INTERFACE)

    get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
    list(APPEND CMAKE_MODULE_PATH ${additional_module_paths})

    get_property(delayed_targets GLOBAL PROPERTY LY_DELAYED_LINK_TARGETS)
    foreach(target ${delayed_targets})
        
        get_property(delayed_link GLOBAL PROPERTY LY_DELAYED_LINK_${target})
        if(delayed_link)
        
            cmake_parse_arguments(ly_delayed_target_link_libraries "" "" "${visibilities}" ${delayed_link})

            foreach(visibility ${visibilities})
                foreach(item ${ly_delayed_target_link_libraries_${visibility}})
                    
                    if(TARGET ${item})
                        get_target_property(item_type ${item} TYPE)
                    else()
                        unset(item_type)
                    endif()

                    if(item_type STREQUAL MODULE_LIBRARY)
                        target_include_directories(${target} ${visibility} $<TARGET_PROPERTY:${item},INTERFACE_INCLUDE_DIRECTORIES>)
                        target_link_libraries(${target} ${visibility} $<TARGET_PROPERTY:${item},INTERFACE_LINK_LIBRARIES>)
                        target_compile_definitions(${target} ${visibility} $<TARGET_PROPERTY:${item},INTERFACE_COMPILE_DEFINITIONS>)
                        target_compile_options(${target} ${visibility} $<TARGET_PROPERTY:${item},INTERFACE_COMPILE_OPTIONS>)
                        # Add it also as a manual dependency so runtime_dependencies walks it through
                        ly_add_dependencies(${target} ${item}) 
                    else()
                        ly_parse_third_party_dependencies(${item})
                        target_link_libraries(${target} ${visibility} ${item})
                    endif()

                endforeach()
            endforeach()
            set_property(GLOBAL PROPERTY LY_DELAYED_LINK_${target})

        endif()

    endforeach()
    set_property(GLOBAL PROPERTY LY_DELAYED_LINK_TARGETS)

endfunction()

#! detect_qt_dependency: Determine if a target will link directly to a Qt library
#
# qt deployment introspects a shared library or executable for its direct
# dependencies on Qt libraries. In CMake, this will be true if a target, or any
# of its link libraries which are static libraries, recursively, links to Qt.
function(detect_qt_dependency TARGET_NAME OUTPUT_VARIABLE)

    if(TARGET ${TARGET_NAME})
        get_target_property(alias ${TARGET_NAME} ALIASED_TARGET)
        if(alias)
            set(TARGET_NAME ${alias})
        endif()
    endif()

    get_property(cached_is_qt_dependency GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${TARGET_NAME})
    if(cached_is_qt_dependency)
        set(${OUTPUT_VARIABLE} ${cached_is_qt_dependency} PARENT_SCOPE)
        return()
    endif()

    if(${TARGET_NAME} MATCHES "^3rdParty::Qt::.*")
        set_property(GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${TARGET_NAME} ON)
        set(${OUTPUT_VARIABLE} ON PARENT_SCOPE)
        return()
    endif()

    get_property(delayed_link GLOBAL PROPERTY LY_DELAYED_LINK_${TARGET_NAME})
    set(exclude_library_types SHARED_LIBRARY MODULE_LIBRARY)
    foreach(library IN LISTS delayed_link)

        if(TARGET ${library})
            get_target_property(child_target_type ${library} TYPE)

            # If the dependency to Qt has to go through a shared/module library,
            # it is not a direct dependency
            if (child_target_type IN_LIST exclude_library_types)
                continue()
            endif()
        endif()

        detect_qt_dependency(${library} child_depends_on_qt)
        if(child_depends_on_qt)
            set_property(GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${TARGET_NAME} ON)
            set(${OUTPUT_VARIABLE} ON PARENT_SCOPE)
            return()
        endif()

    endforeach()

    set_property(GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${TARGET_NAME} OFF)
    set(${OUTPUT_VARIABLE} OFF PARENT_SCOPE)

endfunction()

#! ly_parse_third_party_dependencies: Validates any 3rdParty library dependencies through the find_package command
# 
# \arg:ly_THIRD_PARTY_LIBRARIES name of the target libraries to validate existance of through the find_package command.
#
function(ly_parse_third_party_dependencies ly_THIRD_PARTY_LIBRARIES)
    # Interface dependencies may require to find_packages. So far, we are just using packages for 3rdParty, so we will
    # search for those and automatically bring those packages. The naming convention used is 3rdParty::PackageName::OptionalInterface
    foreach(dependency ${ly_THIRD_PARTY_LIBRARIES})
        string(REPLACE "::" ";" dependency_list ${dependency})
        list(GET dependency_list 0 dependency_namespace)
        if(${dependency_namespace} STREQUAL "3rdParty")
            if (NOT TARGET ${dependency})
                list(GET dependency_list 1 dependency_package)
                ly_download_associated_package(${dependency_package})
                find_package(${dependency_package} REQUIRED MODULE)
            endif()
        endif()
    endforeach()
endfunction()

#! ly_configure_target_platform_properties: Configures any platform specific properties on target
#
# Looks at the the following variables within the platform include file to set the equivalent target properties
# LY_FILES_CMAKE -> extract list of files -> target_sources
# LY_FILES -> target_source
# LY_INCLUDE_DIRECTORIES -> target_include_directories
# LY_COMPILE_DEFINITIONS -> target_compile_definitions
# LY_COMPILE_OPTIONS -> target_compile_options
# LY_LINK_OPTIONS -> target_link_options
# LY_BUILD_DEPENDENCIES -> target_link_libraries
# LY_RUNTIME_DEPENDENCIES -> ly_add_dependencies
# LY_TARGET_PROPERTIES -> target_properties
#
macro(ly_configure_target_platform_properties)
    foreach(platform_include_file ${ly_add_target_PLATFORM_INCLUDE_FILES})
        
        set(LY_FILES_CMAKE)
        set(LY_FILES)
        set(LY_INCLUDE_DIRECTORIES)
        set(LY_COMPILE_DEFINITIONS)
        set(LY_COMPILE_OPTIONS)
        set(LY_LINK_OPTIONS)
        set(LY_BUILD_DEPENDENCIES)
        set(LY_RUNTIME_DEPENDENCIES)
        set(LY_TARGET_PROPERTIES)
        
        include(${platform_include_file} RESULT_VARIABLE ly_platform_cmake_file)
        if(NOT ly_platform_cmake_file)
            message(FATAL_ERROR "The supplied PLATFORM_INCLUDE_FILE(${platform_include_file}) cannot be included.\
 Parsing of target will halt")
        endif()
        if(ly_add_target_HEADERONLY)
            target_sources(${ly_add_target_NAME} INTERFACE ${platform_include_file})
        else()
            target_sources(${ly_add_target_NAME} PRIVATE ${platform_include_file})
        endif()
        ly_source_groups_from_folders("${platform_include_file}")

        if(LY_FILES_CMAKE)
            set(ALLFILES)
            foreach(file_cmake ${LY_FILES_CMAKE})
                ly_include_cmake_file_list(${file_cmake})
            endforeach()
            target_sources(${ly_add_target_NAME} PRIVATE ${ALLFILES})
            ly_source_groups_from_folders("${ALLFILES}")

        endif()
        if(LY_FILES)
            target_sources(${ly_add_target_NAME} PRIVATE ${LY_FILES})
        endif()
        if (LY_INCLUDE_DIRECTORIES)
            target_include_directories(${ly_add_target_NAME} ${LY_INCLUDE_DIRECTORIES})
        endif()
        if(LY_COMPILE_DEFINITIONS)
            target_compile_definitions(${ly_add_target_NAME} ${LY_COMPILE_DEFINITIONS})
        endif()
        if(LY_COMPILE_OPTIONS)
            target_compile_options(${ly_add_target_NAME} ${LY_COMPILE_OPTIONS})
        endif()
        if(LY_LINK_OPTIONS)
            target_link_options(${ly_add_target_NAME} ${LY_LINK_OPTIONS})
        endif()
        if(LY_BUILD_DEPENDENCIES)
            ly_target_link_libraries(${ly_add_target_NAME} ${LY_BUILD_DEPENDENCIES})
        endif()
        if(LY_RUNTIME_DEPENDENCIES)
            ly_add_dependencies(${ly_add_target_NAME} ${LY_RUNTIME_DEPENDENCIES})
        endif()
        if(LY_TARGET_PROPERTIES)
            set_target_properties(${ly_add_target_NAME} PROPERTIES ${LY_TARGET_PROPERTIES})
        endif()
    endforeach()
endmacro()


#! ly_add_target_files: adds files to a target. This will copy the specified files to where the target is.
#
# \arg:TARGETS name of the targets that depends on this file
# \arg:FILES files to copy
# \arg:OUTPUT_SUBDIRECTORY (OPTIONAL) where to place the files relative to TARGET_NAME's output dir. 
#      If not specified, they are located in the same folder as TARGET_NAME
#
function(ly_add_target_files)

    set(options)
    set(oneValueArgs OUTPUT_SUBDIRECTORY)
    set(multiValueArgs TARGETS FILES)

    cmake_parse_arguments(ly_add_target_files "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_target_files_TARGETS)
        message(FATAL_ERROR "You must provide at least one target")
    endif()
    if(NOT ly_add_target_files_FILES)
        message(FATAL_ERROR "You must provide at least a file to copy")
    endif()
   
    foreach(target ${ly_add_target_files_TARGETS})
    
        foreach(file ${ly_add_target_files_FILES})
            set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LY_TARGET_FILES "${file}\n${ly_add_target_files_OUTPUT_SUBDIRECTORY}")
        endforeach()

    endforeach()

endfunction()


#! ly_add_translations: adds translations (ts) to a target.
#
# This wrapper will generate a qrc file with those translations and add the files under "prefix" and add them to
# the indicated targets. These files will be added under the "Generated Files" filter
#
# \arg:TARGETS name of the targets that the translations will be added to
# \arg:PREFIX prefix where the translation will be located within the qrc file
# \arg:FILES translation files to add
#
function(ly_add_translations)

    set(options)
    set(oneValueArgs PREFIX)
    set(multiValueArgs TARGETS FILES)

    cmake_parse_arguments(ly_add_translations "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_translations_TARGETS)
        message(FATAL_ERROR "You must provide at least one target")
    endif()
    if(NOT ly_add_translations_FILES)
        message(FATAL_ERROR "You must provide at least a translation file")
    endif()

    qt5_add_translation(TRANSLATED_FILES ${ly_add_translations_FILES})

    set(qrc_file_contents 
"<RCC>
    <qresource prefix=\"/${ly_add_translations_PREFIX}\">
")
    foreach(file ${TRANSLATED_FILES})
        get_filename_component(filename ${file} NAME)
        string(APPEND qrc_file_contents "        <file>${filename}</file>
")
    endforeach()
    string(APPEND qrc_file_contents "    </qresource>
</RCC>
")
    set(qrc_file_path ${CMAKE_CURRENT_BINARY_DIR}/i18n_${ly_add_translations_PREFIX}.qrc)
    file(WRITE 
        ${qrc_file_path}
        ${qrc_file_contents}
    )
    set_source_files_properties(
            ${TRANSLATED_FILES}
            ${qrc_file_path}
        PROPERTIES 
            GENERATED TRUE
            SKIP_AUTORCC TRUE
    )
    qt5_add_resources(RESOURCE_FILE ${qrc_file_path})

    foreach(target ${ly_add_translations_TARGETS})
        target_sources(${target} PRIVATE "${TRANSLATED_FILES};${qrc_file_path};${RESOURCE_FILE}")
    endforeach()

endfunction()


#! ly_add_source_properties: adds/appends properties to a source file.
#
# This wraps set_source_files_properties to be able to pass a property with multiple values
# and get it appended instead of set.
#
# \arg:SOURCES list of sources to apply this property on
# \arg:PROPERTY property to set
# \arg:VALUES values to append
#
function(ly_add_source_properties)

    set(options)
    set(oneValueArgs PROPERTY)
    set(multiValueArgs SOURCES VALUES)

    cmake_parse_arguments(ly_add_source_properties "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_source_properties_SOURCES)
        message(FATAL_ERROR "You must provide at least one source")
    endif()
    if(NOT ly_add_source_properties_PROPERTY)
        message(FATAL_ERROR "You must provide a property")
    endif()

    foreach(file ${ly_add_source_properties_SOURCES})
        if(NOT IS_ABSOLUTE ${file})
            get_filename_component(file ${file} ABSOLUTE)
        endif()
        if(NOT EXISTS ${file})
            message(SEND_ERROR "File ${file} not found when setting property ${ly_add_source_properties_PROPERTY}")
        endif()
    endforeach()

    # We allow to pass empty values because in some cases we expand the values based on conditions
    # If the values are empty then this call does nothing
    if(ly_add_source_properties_VALUES)
        set_property(
            SOURCE ${ly_add_source_properties_SOURCES}
            APPEND PROPERTY ${ly_add_source_properties_PROPERTY} ${ly_add_source_properties_VALUES}
        )
    endif()

endfunction()


