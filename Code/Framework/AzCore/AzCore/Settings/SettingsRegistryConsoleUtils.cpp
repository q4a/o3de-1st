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

#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Settings/SettingsRegistryConsoleUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::SettingsRegistryConsoleUtils
{
    static void ConsoleSetSettingsRegistryValue(SettingsRegistryInterface& settingsRegistry, const ConsoleCommandContainer& commandArgs)
    {
        if (commandArgs.size() < 2)
        {
            AZ_Error("SettingsRegistryConsoleUtils", false, "At least two arguments must be supplied to the %s command to set a value",
                SettingsRegistrySet);
            return;
        }

        auto commandArgumentsIter = commandArgs.begin();
        // Extract the JSON pointer path from the argument list
        AZ::SettingsRegistryInterface::FixedValueString combinedKeyValueCommand{ *commandArgumentsIter++ };
        combinedKeyValueCommand += "=";
        AZ::StringFunc::Join(combinedKeyValueCommand, commandArgumentsIter, commandArgs.end(), ' ');
        if (settingsRegistry.MergeCommandLineArgument(combinedKeyValueCommand, {}))
        {
            const auto setOutput = AZ::SettingsRegistryInterface::FixedValueString::format(
                R"(Successfully set value at path "%s" into the global settings registry)" "\n",
                combinedKeyValueCommand.c_str());
            AZ::Debug::Trace::Output("SettingsRegistry", setOutput.c_str());
        }
    };

    static void ConsoleRemoveSettingsRegistryValue(SettingsRegistryInterface& settingsRegistry, const ConsoleCommandContainer& commandArgs)
    {
        if (commandArgs.empty())
        {
            AZ_Error("SettingsRegistryConsoleUtils", false, "One or more json paths are required for the %s command."
                " No values will be removed", SettingsRegistryRemove);
            return;
        }

        for (AZStd::string_view commandArg : commandArgs)
        {
            if (settingsRegistry.Remove(commandArg))
            {
                const auto removeOutput = AZ::SettingsRegistryInterface::FixedValueString::format(
                    R"(Successfully removed value at path "%.*s" from the global settings registry)" "\n",
                    aznumeric_cast<int>(commandArg.size()), commandArg.data());
                AZ::Debug::Trace::Output("SettingsRegistry", removeOutput.c_str());
            }
        }
    };

    static void ConsoleDumpSettingsRegistryValue(SettingsRegistryInterface& settingsRegistry, const ConsoleCommandContainer& commandArgs)
    {
        if (commandArgs.empty())
        {
            AZ_Error("SettingsRegistryConsoleUtils", false, "No json pointer paths supplied to the %s command. No values will be dumped",
                SettingsRegistryDump);
            return;
        }

        // Dump each value at the specified json pointer path to an AZStd::string
        constexpr bool prettifyOutput = true;
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings{ prettifyOutput };
        AZStd::string outputString;
        AZ::IO::ByteContainerStream outputStream(&outputString);
        bool appendNewline = false;
        for (AZStd::string_view commandArg : commandArgs)
        {
            if (appendNewline)
            {
                constexpr AZStd::string_view newline{ "\n" };
                outputStream.Write(newline.size(), newline.data());
            }
            if (AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(settingsRegistry, commandArg, outputStream, dumperSettings))
            {
                appendNewline = true;
            }
        }

        AZ::Debug::Trace::Output("SettingsRegistry", outputString.c_str());
    };

    static void ConsoleDumpAllSettingsRegistryValues(SettingsRegistryInterface& settingsRegistry,
        [[maybe_unused]] const ConsoleCommandContainer& commandArgs)
    {
        ConsoleDumpSettingsRegistryValue(settingsRegistry, { "" });
    };

    [[nodiscard]] ConsoleFunctorHandle RegisterAzConsoleCommands(SettingsRegistryInterface& registry, AZ::IConsole& azConsole)
    {
        ConsoleFunctorHandle resultHandle{};
        resultHandle.m_consoleFunctors.emplace_back(azConsole, SettingsRegistrySet,
            R"(Sets a value within the global settings registry at the JSON pointer path @key with value of @value)" "\n"
            R"(@param pointer_path - path to the JSON entry to set)" "\n"
            R"(@param value - JSON value to set at the supplied JSON pointer path)",
            ConsoleFunctorFlags::Null, AZ::TypeId::CreateNull(), registry, &ConsoleSetSettingsRegistryValue);
        resultHandle.m_consoleFunctors.emplace_back(azConsole, SettingsRegistryRemove,
            R"(Removes the value at each provided JSON pointer path from the global settings registry.)" "\n"
            R"(@param pointer_paths - space separated list of path to the JSON entry to remove)",
            ConsoleFunctorFlags::Null, AZ::TypeId::CreateNull(), registry, &ConsoleRemoveSettingsRegistryValue);
        resultHandle.m_consoleFunctors.emplace_back(azConsole, SettingsRegistryDump,
            R"(Dumps one or more values from the global settings registry at input JSON pointer paths)" "\n"
            R"(@param pointer_paths - space separated list of paths to the JSON entry to dump)" "\n",
            ConsoleFunctorFlags::Null, AZ::TypeId::CreateNull(), registry, &ConsoleDumpSettingsRegistryValue);
        resultHandle.m_consoleFunctors.emplace_back(azConsole, SettingsRegistryDumpAll,
            R"(Dumps all values from the global settings registry)" "\n",
            ConsoleFunctorFlags::Null, AZ::TypeId::CreateNull(), registry, &ConsoleDumpAllSettingsRegistryValues);

        return resultHandle;
    }
}
