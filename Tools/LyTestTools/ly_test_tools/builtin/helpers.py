"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Helper file for assisting in building workspaces and setting up LTT with the current Lumberyard environment.
"""

import ly_test_tools._internal.pytest_plugin as pytest_plugin
import ly_test_tools._internal.managers.workspace as internal_workspace
from ly_test_tools import MAC, WINDOWS
import os, stat


def create_builtin_workspace(
        build_directory=None,  # type: str
        project="AutomatedTesting",  # type: str
        tmp_path=None,  # type: str or None
        output_path=None,  # type: str or None
        ):
    # type: (...) -> internal_workspace.AbstractWorkspaceManager
    """
    Create a new platform-specific workspace manager for the current lumberyard build

    :param build_directory: Custom path to the build directory (i.e. engine_root/dev/windows_vs2017/bin/profile)
        when set to None (default) it will use the value configured by pytest CLI argument --build-directory
    :param project: Project name to use
    :param tmp_path: Path to use as temporal storage, if not specified use default
    :param output_path: Path to use as log storage, if not specified use default

    :return: A workspace manager that works with the current lumberyard instance
    """
    if not build_directory:
        if pytest_plugin.build_directory:
            # cannot set argument default (which executes on import), must set here after pytest starts executing
            build_directory = pytest_plugin.build_directory
        else:
            raise ValueError(
                "Cmake build directory was not set via commandline arguments and not overridden. Please specify with "
                r"CLI argument --build-directory (example: --build-directory C:\lumberyard\dev\Win2019\bin\profile )")

    build_class = internal_workspace.AbstractWorkspaceManager
    if WINDOWS:
        from ly_test_tools._internal.managers.platforms.windows import WindowsWorkspaceManager
        build_class = WindowsWorkspaceManager
    elif MAC:
        from ly_test_tools._internal.managers.platforms.mac import MacWorkspaceManager
        build_class = MacWorkspaceManager

    instance = build_class(
        build_directory=build_directory,
        project=project,
        tmp_path=tmp_path,
        output_path=output_path,
    )

    return instance


def setup_bootstrap_project(workspace, project):
    """
    Sets up the bootstrap.cfg file to be used for the given project

    :param workspace: workspace to use
    :param project: Lumberyard project to set as target
    :return: None
    """
    bootstrap_cfg = os.path.join(workspace.paths.dev(), "bootstrap.cfg")
    os.chmod(bootstrap_cfg, stat.S_IWRITE)
    lines = None
    with open(bootstrap_cfg) as f:
        lines = f.readlines()

    found_gamefolder = False
    for i, line in enumerate(lines):
        if line.lstrip().startswith("sys_game_folder"):
            lines[i] = f"sys_game_folder={project}\n"
            found_gamefolder = True
            break

    assert found_gamefolder, "'sys_game_folder' not found in bootstrap.cfg"

    with open(bootstrap_cfg, "w") as f:
        f.writelines(lines)


def setup_builtin_workspace(workspace, test_name, artifact_folder_count):
    # type: (internal_workspace.AbstractWorkspaceManager, str, int) -> internal_workspace.AbstractWorkspaceManager
    """
    Reconfigures a workspace instance to its defaults.
    Usually test authors should rely on the provided "workspace" fixture, but these helpers can be used to
        achieve the same result.

    :param workspace: workspace to use
    :param test_name: the test name to be used by the artifact manager
    :param artifact_folder_count: the number of folders to create for the test_name, each one will have an index
        appended at the end to handle naming collisions.
    :return: the configured workspace object, useful for method chaining
    """
    workspace.setup()
    workspace.artifact_manager.set_test_name(test_name=test_name, amount=artifact_folder_count)

    return workspace


def teardown_builtin_workspace(workspace):
    # type: (internal_workspace.AbstractWorkspaceManager) -> internal_workspace.AbstractWorkspaceManager
    """
    Stop the asset processor and perform teardown on the specified workspace.
    Usually test authors should rely on the provided "workspace" fixture, but these helpers can be used to
        achieve the same result.

    :param workspace: workspace to use
    :return: the configured workspace object, useful for method chaining
    """
    workspace.teardown()

    return workspace
