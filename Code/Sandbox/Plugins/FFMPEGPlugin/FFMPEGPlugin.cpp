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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "FFMPEGPlugin_precompiled.h"
#include "FFMPEGPlugin.h"
#include "CryString.h"
typedef CryStringT<char> string;
#include "Include/ICommandManager.h"
#include "Util/PathUtil.h"

#include <QCoreApplication>
#include <QSettings>
#include <QProcess>

namespace PluginInfo
{
    const char* kName = "FFMPEG Writer";
    const char* kGUID = "{D2A3A44A-00FF-4341-90BA-89A473F44A65}";
    const int kVersion = 1;
}

void CFFMPEGPlugin::Release()
{
    GetIEditor()->GetICommandManager()->UnregisterCommand(COMMAND_MODULE, COMMAND_NAME);
    delete this;
}

void CFFMPEGPlugin::ShowAbout()
{
}

const char* CFFMPEGPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CFFMPEGPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CFFMPEGPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CFFMPEGPlugin::CanExitNow()
{
    return true;
}

static void Command_FFMPEGEncode(const char* input, const char* output, const char* codec, int bitRateinKb, int fps, const char* etc)
{
    QString ffmpegExectablePath = CFFMPEGPlugin::GetFFMPEGExectablePath();
    QString ffmpegCmdLine = QStringLiteral("\"%1\" -r %2 -i \"%3\" -vcodec %4 -b %5k -r %6 %7 -strict experimental -y \"%8\"")
        .arg(ffmpegExectablePath, QString::number(fps), input, codec, QString::number(bitRateinKb), QString::number(fps), etc, output);
    GetIEditor()->GetSystem()->GetILog()->Log("Executing \"%s\" from FFMPEGPlugin...", ffmpegCmdLine.toUtf8().data());
    QString outTxt;
    GetIEditor()->ExecuteConsoleApp(ffmpegCmdLine, outTxt, true, true);
    GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG execution done.");
}

QString CFFMPEGPlugin::GetFFMPEGExectablePath()
{
    // the FFMPEG exe could be in a number of possible locations
    // our preferred location is from the path in the registry @
    // "Software\\Amazon\\Lumberyard\\Settings"
    // in a value called "FFMPEG_PLUGIN"

    // if its not there, try the current folder and a few others
    // see what can be found!
    QString ffmpegExectablePath;

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("Settings");
    ffmpegExectablePath = settings.value("FFMPEG_PLUGIN").toString();

    if (ffmpegExectablePath.isEmpty() || !QFile::exists(ffmpegExectablePath))
    {
        QString path = qApp->applicationDirPath();
        QString checkPath;

        const char* possibleLocations[] = {
            "rc/ffmpeg",
            "editorplugins/ffmpeg",
            "../editor/plugins/ffmpeg",
        };

        for (int idx = 0; idx < 3; ++idx)
        {
#if defined(AZ_PLATFORM_WINDOWS)
            checkPath = QStringLiteral("%1/%2.exe").arg(path, possibleLocations[idx]);
#else
            checkPath = QStringLiteral("%1/%2").arg(path, possibleLocations[idx]);
#endif
            if (QFile::exists(checkPath))
            {
                ffmpegExectablePath = checkPath;
                break;
            }
        }

        if (ffmpegExectablePath.isEmpty())
        {
#if defined(AZ_PLATFORM_WINDOWS)
            ffmpegExectablePath = "ffmpeg.exe";
#else
            ffmpegExectablePath = "ffmpeg";
#endif
            // try the current folder if all else fails, this will also work if its in the PATH somehow.
        }
    }

    return ffmpegExectablePath;
}

bool CFFMPEGPlugin::RuntimeTest()
{
    QString ffmpegExectablePath = CFFMPEGPlugin::GetFFMPEGExectablePath();
    return QProcess::startDetached(QStringLiteral("\"%1\" -version").arg(ffmpegExectablePath), {});
}

void CFFMPEGPlugin::RegisterTheCommand()
{
    CommandManagerHelper::RegisterCommand(GetIEditor()->GetICommandManager(),
        COMMAND_MODULE, COMMAND_NAME, "Encodes a video using ffmpeg.",
        "plugin.ffmpeg_encode 'input.avi' 'result.mov' 'libx264' 200 30",
        functor(Command_FFMPEGEncode));
}

void CFFMPEGPlugin::OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId)
{
}
