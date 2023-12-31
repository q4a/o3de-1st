"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import filecmp
import os
import pytest

# ly_test_tools dependencies.
import ly_remote_console.remote_console_commands as remote_console_commands

from ly_test_tools import WINDOWS

@pytest.fixture
def remote_console(request):
    """
    Creates a RemoteConsole() class instance to send console commands to the
    Lumberyard client console.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :return: ly_remote_console.remote_console_commands.RemoteConsole class instance
        representing the Lumberyard remote console executable.
    """
    # Initialize the RemoteConsole object to send commands to the Lumberyard client console.
    console = remote_console_commands.RemoteConsole()

    # Custom teardown method for this remote_console fixture.
    def teardown():
        console.stop()

    # Utilize request.addfinalizer() to add custom teardown() methods.
    request.addfinalizer(teardown)  # This pattern must be used in pytest version

    return console


@pytest.mark.skipif(not WINDOWS, reason="Editor currently only functions on Windows")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestRemoteConsole(object):

    @pytest.mark.parametrize("project", ["AutomatedTesting"])
    @pytest.mark.parametrize("level", ['Simple'])
    @pytest.mark.parametrize("load_wait", [120])
    def test_RemoteConsole_TakeScreenshot_Success(self, launcher, launcher_platform, remote_console, level, load_wait):
        with launcher.start():
            remote_console.start()
            launcher_load = remote_console.expect_log_line(
                match_string='========================== '
                             'Finished loading textures '
                             '============================',
                timeout=load_wait)

        remote_console_commands.capture_screenshot_command(remote_console)
        assert True
