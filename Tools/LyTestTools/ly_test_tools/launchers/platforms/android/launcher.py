"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Wrapper to manage launching Lumberyard-created apps on Android.

Assumptions for running automation:
-The Android SDK has been installed, and adb is available on the path
-Each attached phone is accessible and:
1. trusts the connected computer (Run "adb devices" in the command line to trust the computer)
2. is unlocked
3. has its USB settings set to File Transfer
"""
import json
import logging
import os
from subprocess import CalledProcessError

import six

import ly_test_tools.mobile.android
import ly_test_tools.environment.process_utils as process_utils

import ly_test_tools.launchers.exceptions
import ly_test_tools.environment.waiter
from ly_test_tools.launchers.platforms.base import Launcher
from ly_test_tools import HOST_OS_PLATFORM

log = logging.getLogger(__name__)


def get_package_name(project_path):
    """
    Gets the Package name from the project's settings JSON.

    :param project_path: The project path of the project
    :return: The Package name from the settings JSON
    """
    project_json_path = os.path.join(project_path, 'project.json')
    with open(project_json_path) as json_file:
        json_list = json.loads(json_file.read())

    try:
        package = json_list['android_settings']['package_name']
    except KeyError as err:
        problem = ly_test_tools.launchers.exceptions.SetupError(
            'Package name not found in {}'.format(project_json_path))
        six.raise_from(problem, err)
    else:
        return package


def get_pid(package_name, adb_prefix):
    """
    Gets the PID for a process running the specified package.

    :param package_name: The package name of the game
    :param adb_prefix: list representing ADB command prefix which is
        generally either ['adb'] or ['adb -s [device ID]']
    :return: The package's process ID if it exists, else None
    """
    # Check android version number; this will fail if a device ID is not set and multiple devices are connected
    version_cmd = []
    version_cmd.extend(adb_prefix)
    version_cmd.extend(['shell', 'getprop', 'ro.build.version.sdk'])
    version = process_utils.check_output(version_cmd)

    # Use the "ps piped to grep" command for API 23 and under, else use the pidof command
    pid_cmd = []
    pid_cmd.extend(adb_prefix)

    if int(version.strip()) >= 24:
        pid_cmd.extend(['shell', 'pidof', package_name])
    else:
        pid_cmd.extend(['shell', 'ps', '|', 'grep', '{}'.format(package_name)])
    try:
        pid = process_utils.check_output(pid_cmd)
    except Exception:  # purposefully broad
        log.exception(f"Exception trying to find a Process ID running package {package_name}, "
                      f"when executing '{pid_cmd}'\n"
                      f"When this occurs, it is possible the application crashed when launched. "
                      "We recommend launching the application manually to verify it is not crashing.",
                      exc_info=True)
        pid = None

    # pidof will only give us an ID, but ps gives multiple items -- we only want the second one in the latter case
    if pid and len(pid.split()) > 1:
        pid = pid.split()[1]

    return pid


def generate_android_map_command(args_list):
    """
    Takes a list of executable args and returns a the android map command to use with the autoexec.cfg file

    :param args_list: list representing args to execute with the current game executable.
    :return: map command to use with android in the autoexec.cfg file
        i.e.: 'map simple_jacklocomotion'
    """
    map_cmd = ''

    for arg in args_list:
        if arg == '+map':
            map_cmd = "{}{}".format('map ', args_list[args_list.index(arg) + 1])

    return map_cmd


class AndroidLauncher(Launcher):
    def __init__(self, workspace, args):
        super(AndroidLauncher, self).__init__(workspace, args)

        self._adb_prefix_command = ['adb']
        self._device_id = None
        self.launch_proc = None
        self.package_name = get_package_name(os.path.join(self.workspace.paths.dev(),
                                                          self.workspace.project))
        self._device_id = self.get_device_config(config_file=self.workspace.paths.devices_file(),
                                                 device_section='android',
                                                 device_key='id')
        log.info('Setting Android device ID: {}'.format(self._device_id))
        self._adb_prefix_command.extend(['-s', self._device_id])
        log.info("Initialized Android Launcher for device ID: {}".format(self._device_id))

    def _enable_android_capabilities(self):
        """
        Enables the required settings for Android device TCP tunnel reversing/forwarding to the host machine.

        :return: None
        """
        # Undo any existing port changes first.
        ly_test_tools.mobile.android.undo_tcp_port_changes(self._device_id)

        # Handle tunneling for Android connections:
        ly_test_tools.mobile.android.reverse_tcp(self._device_id, '61453', '61453')  # Shader Compiler
        ly_test_tools.mobile.android.reverse_tcp(self._device_id, '45643', '45643')  # Asset Processor
        ly_test_tools.mobile.android.forward_tcp(self._device_id, '4600', '4600')  # Remote Console

    def _is_valid_android_environment(self):
        """
        Verifies the current OS can run Android Debug Bridge (ADB) on a connected Android device.

        :return: True if the current environment is valid for Android,
            otherwise raises NotImplementedError or SetupError with the issue encountered.
        """
        if not ly_test_tools.mobile.android.can_run_android():
            raise NotImplementedError(
                f'Android setup not detected on HOST_OS_PLATFORM: "{HOST_OS_PLATFORM}"\n'
                'Setup Android Debug Bridge (ADB) with connected Android device to run Android tests.')

        connected_devices = ly_test_tools.mobile.android.get_devices()
        if not connected_devices:
            raise ly_test_tools.launchers.exceptions.SetupError(
                'No connected devices found when using the "adb devices" command - '
                f'got connected_devices: "{connected_devices}".\n'
                'Please connect an Android device to the host machine and add its ID to the ly_test_tools config file, '
                f'located at: "{self.workspace.paths.devices_file()}"')

        return True

    def setup(self):
        # Backup
        self.backup_settings()

        # Enable Android capabilities and verify environment is setup before continuing.
        self._is_valid_android_environment()
        self._enable_android_capabilities()

        # Modify and re-configure
        self.configure_settings()
        self.workspace.shader_compiler.start()
        super(AndroidLauncher, self).setup()

    def teardown(self):
        ly_test_tools.mobile.android.undo_tcp_port_changes(self._device_id)
        self.restore_settings()
        self.workspace.shader_compiler.stop()
        super(AndroidLauncher, self).teardown()

    def configure_settings(self):
        """
        Configures system level settings and syncs the launcher to the targeted device ID.

        :return: None
        """
        self.workspace.settings.modify_bootstrap_setting('sys_game_folder', self.workspace.project)
        self.workspace.settings.modify_bootstrap_setting('connect_to_remote', 1)
        self.workspace.settings.modify_bootstrap_setting('android_connect_to_remote', 1)
        self.workspace.settings.modify_bootstrap_setting('wait_for_connect', 1)
        self.workspace.settings.modify_bootstrap_setting('remote_ip', '127.0.0.1')
        self.workspace.settings.modify_bootstrap_setting('remote_port', '45643')

        self.workspace.settings.modify_platform_setting('r_AssetProcessorShaderCompiler', 1)
        self.workspace.settings.modify_platform_setting('r_ShadersAsyncCompiling', 0)
        self.workspace.settings.modify_platform_setting('r_ShadersRemoteCompiler', 1)
        self.workspace.settings.modify_platform_setting('r_ShadersAllowCompilation', 1)
        self.workspace.settings.modify_platform_setting('r_ShadersAsyncActivation', 0)
        self.workspace.settings.modify_platform_setting('r_ShaderCompilerServer', '127.0.0.1')
        self.workspace.settings.modify_platform_setting('r_ShaderCompilerPort', '61453')
        self.workspace.settings.modify_platform_setting("log_RemoteConsoleAllowedAddresses", '127.0.0.1')

    def launch(self):
        """
        Launches the APK matching self.package_name to the device that matches self._device_id.

        This method will overwrite any existing autoexec config file on the device if this launcher's args are set,
        which will allow it to do things such as load a level with the "map" command.

        :return: None
        """
        # Handles loading the level with the "map" command.
        autoexec_cfg = self.workspace.paths.autoexec_file()
        file_destination = (
            f'/sdcard/Android/data/{self.package_name}/files/{self.workspace.project}/autoexec.cfg')
        if os.path.isfile(autoexec_cfg):
            ly_test_tools.mobile.android.push_files_to_device(
                source=autoexec_cfg,
                destination=file_destination,
                device=self._device_id
            )

        launch_cmd = []
        launch_cmd.extend(self._adb_prefix_command)
        launch_cmd.extend(['shell',
                           'monkey',
                           '-p',
                           self.package_name,
                           '-c',
                           'android.intent.category.LAUNCHER',
                           '1'])
        try:
            launch_result = process_utils.check_output(launch_cmd)
        except CalledProcessError as error:
            # Format the error to be more human readable:
            error_output = error.output.decode('utf-8').strip().replace('bash arg: ', '')
            raise ly_test_tools.launchers.exceptions.SetupError(
                f'\nGot error output: {error_output.splitlines()}\n'
                f'ADB command used: {error.cmd}\n'
                f'Android APK not located - verify the "{self.package_name}" package is installed.'
            )
        if 'Monkey Aborted' in launch_result:
            log.error(f'Android APK launch failed! Command executed was "{launch_cmd}" with output: {launch_result}')
            raise ly_test_tools.launchers.exceptions.SetupError(
                f'Android APK launch failed immediately for command "{launch_cmd}"')
        else:
            log.debug(f"Started Android Launcher with command: {launch_cmd}")

    def is_alive(self):
        """
        Checks that the package matching self.package_name is running on the device matching self._device_id

        :return: whether a process for the stored package name is currently running on a connected device
        """
        if get_pid(self.package_name, self._adb_prefix_command):
            return True
        return False

    def kill(self):
        """
        Attempts to force quit any running processes with the stored package name on the device
        that is set to self._device_id via the self._adb_prefix_command

        :return: None
        """
        # Using certain ADB commands will throw if multiple devices are connected and device_id is not yet set
        forcestop_cmd = []
        forcestop_cmd.extend(self._adb_prefix_command)
        forcestop_cmd.extend(['shell',
                              'am',
                              'force-stop',
                              self.package_name])
        process_utils.check_call(forcestop_cmd)
        log.debug("Android Launcher terminated successfully")

    def binary_path(self):
        raise NotImplementedError("Android does not have a binary path")
