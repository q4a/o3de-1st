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

include(cmake/Platform/Common/Configurations_common.cmake)
include(cmake/Platform/Common/VisualStudio_common.cmake)

set(LY_MSVC_SUPPORTED_GENERATORS
    "Visual Studio 15"
    "Visual Studio 16"
)
set(FOUND_SUPPORTED_GENERATOR)
foreach(supported_generator ${LY_MSVC_SUPPORTED_GENERATORS})
    if(CMAKE_GENERATOR MATCHES ${supported_generator})
        set(FOUND_SUPPORTED_GENERATOR TRUE)
        break()
    endif()
endforeach()
# VS2017's checks since it defaults the toolchain and target architecture to x86
if(CMAKE_GENERATOR MATCHES "Visual Studio 15")
    if(CMAKE_VS_PLATFORM_NAME AND CMAKE_VS_PLATFORM_NAME STREQUAL "Win32") # VS2017 has Win32 as the default architecture
        message(FATAL_ERROR "Win32 architecture not supported, specify \"-A x64\" when invoking cmake")
    endif()
    if(NOT CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x64") # There is at least one library (EditorLib) that make the x86 linker to run out of memory
        message(FATAL_ERROR "x86 toolset not supported, specify \"-T host=x64\" when invoking cmake")
    endif()
else()
    # For the other cases, verify that it wasn't invoked with an unsupported architecture. defaults to x86 architecture
    if(SUPPORTED_VS_PLATFORM_NAME_OVERRIDE)
        set(SUPPORTED_VS_PLATFORM_NAME ${SUPPORTED_VS_PLATFORM_NAME_OVERRIDE})
    else()
        set(SUPPORTED_VS_PLATFORM_NAME x64)
    endif()

    if(CMAKE_VS_PLATFORM_NAME AND NOT CMAKE_VS_PLATFORM_NAME STREQUAL "${SUPPORTED_VS_PLATFORM_NAME}")
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} architecture not supported")
    endif()
    if(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE AND NOT CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x64")
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE} toolset not supported")
    endif()
endif()

ly_append_configurations_options(
    DEFINES
        _ENABLE_EXTENDED_ALIGNED_STORAGE #Enables support for extended alignment for the MSVC std::aligned_storage class
    COMPILATION
        /nologo         # Suppress Copyright and version number message
        /fp:fast        # allows the compiler to reorder, combine, or simplify floating-point operations to optimize floating-point code for speed and space
        /Gd             # Use _cdecl calling convention for all functions
        /MP             # Multicore compilation in Visual Studio
        /nologo         # Suppress Copyright and version number message
        /W4             # Warning level 4
        /WX             # Warnings as errors
        
        # Disabling these warnings while they get fixed
        /wd4018 # signed/unsigned mismatch
        /wd4115 # named type definition in parentheses
        /wd4121 # alignment of a member was sensitive to packing
        /wd4189 # local variable is initialized but not referenced
        /wd4201 # nonstandard extension used: nameless struct/union
        /wd4211 # nonstandard extension used: redefined extern to static
        /wd4232 # nonstandard extension used: address of dllimport is not static, identity not guaranteed
        /wd4238 # nonstandard extension used: class rvalue used as lvalue
        /wd4244 # conversion, possible loss of data
        /wd4245 # conversion, signed/unsigned mismatch
        /wd4267 # conversion, possible loss of data
        /wd4310 # cast truncates constant value
        /wd4324 # structure was padded due to alignment specifier
        /wd4366 # the result of unary operator may be unaligned
        /wd4373 # previous versions of the compiler did not override when parameters only differed by const/volatile qualifiers
        /wd4389 # comparison, signed/unsigned mismatch
        /wd4436 # the result of unary operator may be unaligned
        /wd4450 # declaration hides global declaration
        /wd4457 # declaration hides function parameter
        /wd4459 # declaration hides global declaration
        /wd4463 # overflow, assigning 1 to bit-field can only hold values from -1 to 0
        /wd4701 # potentially unintialized local variable
        /wd4702 # unreachable code
        /wd4703 # potentially unitialized local pointer variable
        /wd4706 # assignment with conditional expression
        /wd4913 # user defined binary operator ',' exists but no overload could convert all operands

        # Enabling warnings that are disabled by default from /W4
        # https://docs.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default?view=vs-2019
        # /we4296 # 'operator': expression is always false
        # /we4426 # optimization flags changed after including header, may be due to #pragma optimize()
        # /we4464 # relative include path contains '..'
        # /we4619 # #pragma warning: there is no warning number 'number'
        # /we4777 # 'function' : format string 'string' requires an argument of type 'type1', but variadic argument number has type 'type2' looks useful
        # /we5031 # #pragma warning(pop): likely mismatch, popping warning state pushed in different file
        # /WE5032 # detected #pragma warning(push) with no corresponding #pragma warning(pop)

        /Zc:forScope    # Force Conformance in for Loop Scope
        /diagnostics:caret # Compiler diagnostic options: includes the column where the issue was found and places a caret (^) under the location in the line of code where the issue was detected.
        /Zc:__cplusplus
        /favor:AMD64    # Create Code optimized for 64 bit
        /bigobj         # Increase number of sections in obj files. Profiling has shown no meaningful impact in memory nore build times
    COMPILATION_DEBUG
        /GS             # Enable Buffer security check
        /MDd            # defines _DEBUG, _MT, and _DLL and causes the application to use the debug multithread-specific and DLL-specific version of the run-time library. 
                        # It also causes the compiler to place the library name MSVCRTD.lib into the .obj file.
        /Ob0            # Disables inline expansions
        /Od             # Disables optimization
        /RTCsu          # Run-Time Error Checks: c Reports when a value is assigned to a smaller data type and results in a data loss (Not supoported by the STL)
                        #                        s Enables stack frame run-time error checking
                        #                        u Reports when a variable is used without having been initialized
    COMPILATION_PROFILE
        /GF             # Enable string pooling   
        /Gy             # Function level linking
        /MD             # Causes the application to use the multithread-specific and DLL-specific version of the run-time library. Defines _MT and _DLL and causes the compiler 
                        # to place the library name MSVCRT.lib into the .obj file.
        /O2             # Maximinize speed, equivalent to /Og /Oi /Ot /Oy /Ob2 /GF /Gy
        /Zc:inline      # Removes unreferenced functions or data that are COMDATs or only have internal linkage
        /Zc:wchar_t     # Use compiler native wchar_t
        /Zi             # Generate debugging information (no Edit/Continue)
    COMPILATION_RELEASE
        /Ox             # Full optimization
        /Ob2            # Inline any suitable function
        /Ot             # Favor fast code over small code
        /Oi             # Use Intrinsic Functions
        /Oy             # Omit the frame pointer
    LINK
        /NOLOGO             # Suppress Copyright and version number message
        /IGNORE:4099        # 3rdParty linking produces noise with LNK4099
    LINK_NON_STATIC_PROFILE
        /OPT:REF            # Eliminates functions and data that are never referenced
        /OPT:ICF            # Perform identical COMDAT folding. Redundant COMDATs can be removed from the linker output
        /INCREMENTAL:NO
        /DEBUG              # Generate pdbs
    LINK_NON_STATIC_RELEASE
        /OPT:REF # Eliminates functions and data that are never referenced
        /OPT:ICF # Perform identical COMDAT folding. Redundant COMDATs can be removed from the linker output
        /INCREMENTAL:NO
)

set(LY_BUILD_WITH_INCREMENTAL_LINKING_DEBUG TRUE CACHE BOOL "Indicates if incremental linking is used in debug configurations (default = TRUE)")
if(LY_BUILD_WITH_INCREMENTAL_LINKING_DEBUG)
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /ZI         # Enable Edit/Continue
    )
else()
    # Disable incremental linking
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /Zi         # Generate debugging information (no Edit/Continue). Edit/Continue requires incremental linking
        LINK_NON_STATIC_DEBUG
            /DEBUG      # Despite the documentation states /Zi implies /DEBUG, without it, stack traces are not expanded
            /INCREMENTAL:NO

    )    
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio 15")
    # Visual Studio 2017 has problems with [[maybe_unused]] on lambdas. Sadly, there is no different warning, so 4100 has to remain disabled on 2017
    ly_append_configurations_options(
        COMPILATION
            /wd4100
    )
endif()

# Configure system includes
set(LY_CXX_SYSTEM_INCLUDE_CONFIGURATION_FLAG 
    /experimental:external # Turns on "external" headers feature for MSVC compilers
    /external:W0 # Set warning level in external headers to 0. This is used to suppress warnings 3rdParty libraries which uses the "system_includes" option in their json configuration
    /wd4193 # Temporary workaround for the /experiment:external feature generating warning C4193: #pragma warning(pop): no matching '#pragma warning(push)'
)
if(NOT CMAKE_INCLUDE_SYSTEM_FLAG_CXX)
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX /external:I)
endif()

include(cmake/Platform/Common/TargetIncludeSystemDirectories_unsupported.cmake)

