"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

LySettings provides an API for modifying settings files and creating/restoring backups of them.
"""

import fileinput
import logging
import re
import os

import ly_test_tools.environment.file_system

logger = logging.getLogger(__name__)


class LySettings(object):
    """
    LySettings provides an API for modifying settings files and creating/restoring backups of them.
    """

    def __init__(self, temp_path, resource_locator):
        self._temp_path = temp_path
        self._resource_locator = resource_locator

    def get_temp_path(self):
        return self._temp_path

    def modify_asset_processor_setting(self, setting, value):
        logger.info(f'Updating setting {setting} to {value}')
        _edit_text_settings_file(self._resource_locator.asset_processor_config_file(), setting, value)

    def modify_platform_setting(self, setting, value):
        logger.info(f'Updating setting {setting} to {value}')
        _edit_text_settings_file(self._resource_locator.platform_config_file(), setting, value)

    def modify_bootstrap_setting(self, setting, value, bootstrap_path=None):
        logger.info(f'Updating setting {setting} to {value}')
        _edit_text_settings_file(bootstrap_path or self._resource_locator.bootstrap_config_file(), setting, value)

    def modify_shader_compiler_setting(self, setting, value):
        logger.info(f'Updating setting {setting} to {value}')
        _edit_text_settings_file(self._resource_locator.shader_compiler_config_file(), setting, value)

    def backup_asset_processor_settings(self, backup_path=None):
        self._backup_settings(self._resource_locator.asset_processor_config_file(), backup_path)

    def backup_platform_settings(self, backup_path=None):
        """
        Creates a backup of the platform settings file (~/dev/system_[platform].cfg) in the backup_path. If no path is
        provided, it will store in the workspace temp path (the contents of the workspace temp directory are removed
        during workspace teardown)
        """
        self._backup_settings(self._resource_locator.platform_config_file(), backup_path)

    def backup_bootstrap_settings(self, backup_path=None):
        """
        Creates a backup of the bootstrap settings file (~/dev/bootstrap.cfg) in the backup_path. If no path is
        provided, it will store in the workspace temp path (the contents of the workspace temp directory are removed
        during workspace teardown)
        """
        self._backup_settings(self._resource_locator.bootstrap_config_file(), backup_path)

    def backup_shader_compiler_settings(self, backup_path=None):
        self._backup_settings(self._resource_locator.shader_compiler_config_file(), backup_path)

    def restore_asset_processor_settings(self, backup_path):
        self._restore_settings(self._resource_locator.asset_processor_config_file(), backup_path)

    def restore_platform_settings(self, backup_path=None):
        """
        Restores the platform settings file (~/dev/system_[platform].cfg) from its backup.
        The backup is stored in the backup_path.
        If no backup_path is provided, it will attempt to retrieve the backup from the workspace temp path.
        """
        self._restore_settings(self._resource_locator.platform_config_file(), backup_path)

    def restore_bootstrap_settings(self, backup_path=None):
        """
        Restores the bootstrap settings file (~/dev/bootstrap.cfg) from its backup.
        The backup is stored in the backup_path.
        If no backup_path is provided, it will attempt to retrieve the backup from the workspace temp path.
        """
        self._restore_settings(self._resource_locator.bootstrap_config_file(), backup_path)

    def restore_shader_compiler_settings(self, backup_path=None):
        self._restore_settings(self._resource_locator.shader_compiler_config_file(), backup_path)

    def _backup_settings(self, settings_file, backup_path):
        """
        Creates a backup of the settings file in the backup_path. If no path is
        provided, it will store in the workspace temp path (the contents of the workspace temp directory are removed
        during workspace teardown)
        """
        if not backup_path:
            backup_path = self._temp_path
        ly_test_tools.environment.file_system.create_backup(settings_file, backup_path)

    def _restore_settings(self, settings_file, backup_path):
        """
        Restores the settings file from its backup stored in backup_path. If no path is provided, it will attempt
        to retrieve the backup from the workspace temp path.
        """
        if not backup_path:
            backup_path = self._temp_path
        ly_test_tools.environment.file_system.restore_backup(settings_file, backup_path)


def _edit_text_settings_file(settings_file, setting, value, comment_char=""):
    """
    Find and set a specific setting in a text based settings file.  Uses "setting = value" syntax to identify setting,
    ignoring whitespace.  Will append "setting = value" to a text file if it can not find the setting already.

    Note all found instances of the setting key in the file will be changed.
    Unintentional setting changes may happen for files with multiple settings named the same

    Using the comment_char, users can set a value on a corresponding setting but leave it commented out
    --setting=value

    :param settings_file: The path to a settings file
    :param setting: The target key setting to update
    :param value: The new key value
    :param comment_char: A character identifier for commenting out data in a settings file
    """

    if not os.path.isfile(settings_file):
        raise IOError(f"Invalid file and/or path {settings_file}.")

    match_obj = None
    document = None
    try:
        # fileinput can be very destructive when used in conjunction with "with as" due to temp file creation.
        # Allows using print to rewrite lines.
        logger.debug(f"Opening {settings_file}")
        document = fileinput.input(settings_file, inplace=True)
        for line in document:
            # Remove whitespace to avoid double spacing the output.
            line = line.rstrip()
            if setting in line:
                # Run regex on each line, and make the change if we have an exact match.
                setting_regex = re.compile("([-;]+)?(.*)=(.*)")
                possible_match = setting_regex.match(line)
                if possible_match is None or setting != possible_match.group(2).strip():
                    print(line)
                    continue
                match_obj = possible_match
                logger.debug(f"Found {setting} in {settings_file}")
                # Print the new value for the setting.
                if value == "":
                    print("")
                else:
                    print(f"{comment_char}{setting}={value}")
                logger.info(f"Updated setting {setting} in {settings_file} to {value} from {match_obj.group(3)}")
            else:
                print(line)

    except PermissionError as error:
        logger.warning(f"PermissionError, possibly due to ({settings_file}) already being open. Error: {error}")
    finally:
        if document is not None:
            document.close()

    # Append the settings change if setting doesn't exist in file.
    if match_obj is None:
        logger.info(
            "Unable to locate setting in file. "
            f"Appending {comment_char}{setting}={value} to {settings_file}.")
        with open(settings_file, "a") as document:
            document.write(f"{comment_char}{setting}={value}")
