/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "DebugOutput.h"

namespace AZ::SceneAPI::Utilities
{
    void DebugOutput::Write(const char* name, const char* data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data);
    }

    void DebugOutput::Write(const char* name, const AZStd::string& data)
    {
        Write(name, data.c_str());
    }

    void DebugOutput::Write(const char* name, double data)
    {
        m_output += AZStd::string::format("\t%s: %f\n", name, data);
    }

    void DebugOutput::Write(const char* name, uint64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRIu64 "\n", name, data);
    }

    void DebugOutput::Write(const char* name, int64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRId64 "\n", name, data);
    }

    void DebugOutput::Write(const char* name, const DataTypes::MatrixType& data)
    {
        AZ::Vector3 basisX{};
        AZ::Vector3 basisY{};
        AZ::Vector3 basisZ{};
        AZ::Vector3 translation{};
        data.GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);

        m_output += AZStd::string::format("\t%s:\n", name);

        m_output += "\t";
        Write("BasisX", basisX);

        m_output += "\t";
        Write("BasisY", basisY);

        m_output += "\t";
        Write("BasisZ", basisZ);

        m_output += "\t";
        Write("Transl", translation);
    }

    void DebugOutput::Write(const char* name, bool data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data ? "true" : "false");
    }

    void DebugOutput::Write(const char* name, Vector3 data)
    {
        m_output += AZStd::string::format("\t%s: <% f, % f, % f>\n", name, data.GetX(), data.GetY(), data.GetZ());
    }

    const AZStd::string& DebugOutput::GetOutput() const
    {
        return m_output;
    }
}
