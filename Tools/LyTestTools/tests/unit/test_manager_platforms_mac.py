"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for ly_test_tools._internal.managers.platforms.mac
"""

import unittest.mock as mock
import os
import pytest

from ly_test_tools._internal.managers.platforms.mac import (
    _MacResourceLocator, MacWorkspaceManager,
    CACHE_DIR, CONFIG_FILE)
from ly_test_tools import MAC

pytestmark = pytest.mark.SUITE_smoke

if not MAC:
    pytestmark = pytest.mark.skipif(
        not MAC,
        reason="test_manager_platforms_mac.py only runs on Mac")

mock_engine_root = 'mock_engine_root'
mock_dev_path = 'mock_dev_path'
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_tmp_path = 'mock_tmp_path'
mock_output_path = 'mock_output_path'

mac_resource_locator = _MacResourceLocator(
    build_directory=mock_build_directory,
    project=mock_project)


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class TestMacResourceLocator(object):

    def test_PlatformConfigFile_HasPath_ReturnsPath(self):
        expected = os.path.join(
            mac_resource_locator.dev(),
            CONFIG_FILE)

        assert mac_resource_locator.platform_config_file() == expected

    def test_PlatformCache_HasPath_ReturnsPath(self):
        expected = os.path.join(
            mac_resource_locator.project_cache(),
            CACHE_DIR)

        assert mac_resource_locator.platform_cache() == expected

    def test_ProjectLog_HasPath_ReturnsPath(self):
        expected = os.path.join(
            mac_resource_locator.platform_cache(),
            'user',
            'log')

        assert mac_resource_locator.project_log() == expected

    def test_ProjectScreenshots_HasPath_ReturnsPath(self):
        expected = os.path.join(
            mac_resource_locator.platform_cache(),
            'user',
            'ScreenShots')

        assert mac_resource_locator.project_screenshots() == expected

    def test_EditorLog_HasPath_ReturnsPath(self):
        expected = os.path.join(
            mac_resource_locator.project_log(),
            'editor.log')

        assert mac_resource_locator.editor_log() == expected


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class TestMacWorkspaceManager(object):

    def test_Init_SetDummyParams_ReturnsMacWorkspaceManager(self):
        mac_workspace_manager = MacWorkspaceManager(
            build_directory=mock_build_directory,
            project=mock_project,
            tmp_path=mock_tmp_path,
            output_path=mock_output_path)

        assert type(mac_workspace_manager) == MacWorkspaceManager
        assert mac_workspace_manager.paths.build_directory() == mock_build_directory
        assert mac_workspace_manager.paths._project == mock_project
        assert mac_workspace_manager.tmp_path == mock_tmp_path
        assert mac_workspace_manager.output_path == mock_output_path
