"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit Tests for ly_test_tools._internal.managers.abstract_resource_locator
"""
import os

import unittest.mock as mock
import pytest

import ly_test_tools._internal.managers.abstract_resource_locator as abstract_resource_locator

pytestmark = pytest.mark.SUITE_smoke

mock_initial_path = "mock_initial_path"
mock_engine_root = "mock_engine_root"
mock_dev_path = "mock_dev_path"
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'


class TestFindEngineRoot(object):

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath')
    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.exists')
    def test_FindEngineRoot_InitialPathExists_ReturnsTuple(self, mock_path_exists, mock_abspath):
        mock_path_exists.return_value = True
        mock_abspath.return_value = mock_engine_root

        engine_root, dev_path = abstract_resource_locator._find_engine_root(mock_initial_path)

        assert engine_root == mock_engine_root
        assert dev_path == mock_initial_path
        mock_path_exists.assert_called_once()
        mock_abspath.assert_called_once()

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath')
    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.exists')
    def test_FindEngineRoot_InitialPathDoesntExist_RaisesOSError(self, mock_path_exists, mock_abspath):
        mock_path_exists.return_value = False
        mock_abspath.return_value = mock_engine_root

        with pytest.raises(OSError):
            abstract_resource_locator._find_engine_root(mock_initial_path)


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath',
            mock.MagicMock(return_value=mock_initial_path))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class TestAbstractResourceLocator(object):

    def test_Init_HasEngineRoot_SetsAttrs(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator._build_directory == mock_build_directory
        assert mock_abstract_resource_locator._engine_root == mock_engine_root
        assert mock_abstract_resource_locator._dev_path == mock_dev_path
        assert mock_abstract_resource_locator._project == mock_project

    def test_BasePath_IsCalled_ReturnsBasePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.engine_root() == mock_engine_root

    def test_Dev_IsCalled_ReturnsDevPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.dev() == mock_dev_path

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.isfile')
    def test_3rdParty_IsCalledHasTxtFile_Returns3rdPartyPath(self, mock_isfile):
        mock_isfile.return_value = True
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator._engine_root, '3rdParty')

        assert mock_abstract_resource_locator.third_party() == expected_path

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.isfile')
    def test_3rdParty_IsCalledNoTxtFile_RaisesFileNotFoundError(self, mock_isfile):
        mock_isfile.return_value = False
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        with pytest.raises(FileNotFoundError):
            mock_abstract_resource_locator.third_party()
            mock_isfile.assert_called_once()

    def test_BuildDirectory_IsCalled_ReturnsBuildDirectoryPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.build_directory() == mock_build_directory

    def test_Project_IsCalled_ReturnsProjectPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(), mock_project)

        assert mock_abstract_resource_locator.project() == expected_path

    def test_AssetProcessor_IsCalled_ReturnsAssetProcessorPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'AssetProcessor')

        assert mock_abstract_resource_locator.asset_processor() == expected_path

    def test_AssetProcessorBatch_IsCalled_ReturnsAssetProcessorBatchPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'AssetProcessorBatch')

        assert mock_abstract_resource_locator.asset_processor_batch() == expected_path

    def test_Editor_IsCalled_ReturnsEditorPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'Editor')

        assert mock_abstract_resource_locator.editor() == expected_path

    def test_Cache_IsCalled_ReturnsCachePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(), 'Cache')

        assert mock_abstract_resource_locator.cache() == expected_path

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.exists')
    def test_GetShaderCompilerPath_IsCalledExecutablePathExists_ReturnsGetShaderCompilerPath(self, mock_path_exists):
        mock_path_exists.return_value = True
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'CrySCompileServer')

        assert mock_abstract_resource_locator.get_shader_compiler_path() == expected_path

    def test_GetShaderCompilerDir_IsCalled_ReturnsShaderCompilerDir(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = mock_abstract_resource_locator.build_directory()

        assert mock_build_directory == expected_path

    def test_ShaderCompilerConfigFile_IsCalled_ReturnsShaderCompilerConfigPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'config.ini')

        assert mock_abstract_resource_locator.shader_compiler_config_file() == expected_path

    def test_ShaderCache_IsCalled_ReturnsShaderCacheDir(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'Cache')

        assert mock_abstract_resource_locator.shader_cache() == expected_path

    def test_BootstrapConfigFile_IsCalled_ReturnBootstrapConfigFilePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(), 'bootstrap.cfg')

        assert mock_abstract_resource_locator.bootstrap_config_file() == expected_path

    def test_AssetProcessorConfigFile_IsCalled_ReturnsAssetProcessorConfigFilePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(), 'AssetProcessorPlatformConfig.ini')

        assert mock_abstract_resource_locator.asset_processor_config_file() == expected_path

    def test_AutoexecFile_IsCalled_ReturnsAutoexecFilePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(),
                                     mock_abstract_resource_locator._project,
                                     'autoexec.cfg')

        assert mock_abstract_resource_locator.autoexec_file() == expected_path

    def test_TestResults_IsCalled_TestResultsPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.dev(), 'TestResults')

        assert mock_abstract_resource_locator.test_results() == expected_path

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.expanduser')
    def test_DevicesFile_IsCalled_ReturnsDevicesFilePath(self, mock_expanduser):
        mock_expanded_path = 'C:/somepath/'
        mock_expanduser.return_value = mock_expanded_path
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_expanded_path, 'ly_test_tools', 'devices.ini')

        assert mock_abstract_resource_locator.devices_file() == expected_path

    def test_PlatformConfigFile_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.platform_config_file()

    def test_PlatformCache_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.platform_cache()

    def test_ProjectLog_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.project_log()

    def test_ProjectScreenshots_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.project_screenshots()

    def test_EditorLog_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.editor_log()
