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

import fnmatch
import os
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

_SINGLE_NEWLINE_ENDING_REGEX = re.compile(r'\n\Z', re.MULTILINE)
_MULTI_NEWLINE_ENDING_REGEX = re.compile(r'\n\s*\n\Z', re.MULTILINE)
_CRLF_REGEX = re.compile(r'^.*\r\n', re.MULTILINE)
_ONLY_LF_REGEX = re.compile(r'^[^\r]*\n', re.MULTILINE)


class NewlineValidator(CommitValidator):
    """A file-level validator that makes sure a file contains valid line-endings"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            file_identifier = f"{file_name}::{self.__class__.__name__}"
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_identifier} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                file_extension = os.path.splitext(file_name)[1].lower()
                if IsFileSkipped(file_name):
                    if VERBOSE: print(f'{file_identifier} SKIPPED - File excluded based on extension.')
                    continue

                # since this validator focuses on newlines throughout the file, not just in diffs
                # we use the real file data instead of a diff
                with open(file_name, 'rt', encoding='utf8', errors='replace') as fh:
                    lines = fh.read()
                    if not _SINGLE_NEWLINE_ENDING_REGEX.search(lines):
                        error_message = str(f'{file_identifier} FAILED - Source file does not end with a trailing newline.')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)

                    if _MULTI_NEWLINE_ENDING_REGEX.search(lines):
                        error_message = str(f'{file_identifier} FAILED - Source file ends in multiple trailing newlines.')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)

                    crlf_result = _CRLF_REGEX.search(lines)
                    only_lf_result = _ONLY_LF_REGEX.search(lines)
                    if crlf_result and only_lf_result:
                        error_message = str(f'{file_identifier} FAILED - Source file contains mixed line endings (\\r\\n and \\n)')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)

                    if crlf_result:
                        error_message = str(f'{file_identifier} FAILED - Source file incorrectly contains Windows-style line endings'
                              f' (\\r\\n), when Unix-style line endings (\\n) were expected.  Enable git option '
                              f'"core.autocrlf" to avoid this error.')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)

        return (not errors)


def get_validator() -> Type[NewlineValidator]:
    """Returns the validator class for this module"""
    return NewlineValidator
