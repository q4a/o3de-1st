"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for ly_test_tools._internal.managers.platforms.windows
"""

import unittest.mock as mock
import os
import pytest

from ly_test_tools._internal.managers.platforms.windows import (
    _WindowsResourceLocator, WindowsWorkspaceManager,
    CACHE_DIR, CONFIG_FILE)
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_smoke

if not WINDOWS:
    pytestmark = pytest.mark.skipif(
        not WINDOWS,
        reason="test_manager_platforms_windows.py only runs on Windows")

mock_engine_root = 'mock_engine_root'
mock_dev_path = 'mock_dev_path'
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_tmp_path = 'mock_tmp_path'
mock_output_path = 'mock_output_path'

windows_resource_locator = _WindowsResourceLocator(
    build_directory=mock_build_directory,
    project=mock_project)

windows_workspace_manager = WindowsWorkspaceManager(
    build_directory=mock_build_directory,
    project=mock_project,
    tmp_path=mock_tmp_path,
    output_path=mock_output_path)


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class TestWindowsResourceLocator(object):

    def test_PlatformConfigFile_HasPath_ReturnsPath(self):
        expected = os.path.join(
            windows_resource_locator.dev(),
            CONFIG_FILE)

        assert windows_resource_locator.platform_config_file() == expected

    def test_PlatformCache_HasPath_ReturnsPath(self):
        expected = os.path.join(
            windows_resource_locator.project_cache(), CACHE_DIR)

        assert windows_resource_locator.platform_cache() == expected

    def test_ProjectLog_HasPath_ReturnsPath(self):
        expected = os.path.join(
            windows_resource_locator.platform_cache(),
            'user',
            'log')

        assert windows_resource_locator.project_log() == expected

    def test_ProjectScreenshots_HasPath_ReturnsPath(self):
        expected = os.path.join(
            windows_resource_locator.platform_cache(),
            'user',
            'ScreenShots')

        assert windows_resource_locator.project_screenshots() == expected

    def test_EditorLog_HasPath_ReturnsPath(self):
        expected = os.path.join(
            windows_resource_locator.project_log(),
            'editor.log')

        assert windows_resource_locator.editor_log() == expected


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class TestWindowsWorkspaceManager(object):

    @mock.patch('ly_test_tools.environment.reg_cleaner.create_ly_keys')
    def test_SetRegistryKeys_NewWorkspaceManager_KeyCreateCalled(self, mock_create_keys):
        windows_workspace_manager.set_registry_keys()

        mock_create_keys.assert_called_once()

    @mock.patch('ly_test_tools.environment.reg_cleaner.clean_ly_keys')
    def test_ClearSettings_NewWorkspaceManager_KeyClearCalled(self, mock_clear_keys):
        windows_workspace_manager.clear_settings()

        mock_clear_keys.assert_called_with(exception_list=r"SOFTWARE\Amazon\Lumberyard\Identity")
