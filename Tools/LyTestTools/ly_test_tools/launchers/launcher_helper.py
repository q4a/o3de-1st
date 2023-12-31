"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Main launchers module, provides a facade for creating launchers.
"""

import logging

import ly_test_tools._internal.managers.workspace
import ly_test_tools

log = logging.getLogger(__name__)


def create_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_PLATFORM, args=None):
    # type: (ly_test_tools.managers.workspace.WorkspaceManager, str, List[str]) -> Launcher
    """
    Create a launcher compatible with the specified workspace, if no specific launcher is found return a generic one.

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows' or 'android')
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform, ly_test_tools.HOST_OS_PLATFORM)
    return launcher_class(workspace, args)


def create_dedicated_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_DEDICATED_SERVER, args=None):
    # type: (ly_test_tools.managers.workspace.WorkspaceManager, str, List[str]) -> Launcher
    """
    Create a dedicated launcher compatible with the specified workspace.  Dedicated Launcher is only supported on the
    Linux and Windows Platform

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows_dedicated' for DedicatedWinLauncher)
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform, ly_test_tools.HOST_OS_DEDICATED_SERVER)
    return launcher_class(workspace, args)


def create_editor(workspace, launcher_platform=ly_test_tools.HOST_OS_EDITOR, args=None):
    # type: (ly_test_tools.managers.workspace.WorkspaceManager, str, List[str]) -> Launcher
    """
    Create an Editor compatible with the specified workspace.
    Editor is only officially supported on the Windows Platform.

    :param workspace: lumberyard workspace to use
     :param launcher_platform: the platform to target for a launcher (i.e. 'windows_dedicated' for DedicatedWinLauncher)
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Editor instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform, ly_test_tools.HOST_OS_EDITOR)
    return launcher_class(workspace, args)
