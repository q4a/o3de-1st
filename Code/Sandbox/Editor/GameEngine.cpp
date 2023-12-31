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

#include "EditorDefs.h"

#include "GameEngine.h"

// Qt
#include <QMessageBox>
#include <QThread>

// AzCore
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/parallel/binary_semaphore.h>

// AzFramework
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>      // for TerrainDataRequests
#include <AzFramework/Archive/IArchive.h>

// Editor
#include "IEditorImpl.h"
#include "CryEditDoc.h"
#include "Geometry/EdMesh.h"
#include "Mission.h"
#include "Settings.h"

// CryCommon
#include <CryCommon/I3DEngine.h>
#include <CryCommon/INavigationSystem.h>
#include <CryCommon/IDeferredCollisionEvent.h>
#include <CryCommon/ITimeOfDay.h>
#include <CryCommon/LyShine/ILyShine.h>
#include <CryCommon/ParseEngineConfig.h>
#include <CryCommon/MainThreadRenderRequestBus.h>

// Editor
#include "CryEdit.h"

#include "ViewManager.h"
#include "Util/Ruler.h"
#include "AnimationContext.h"
#include "UndoViewPosition.h"
#include "UndoViewRotation.h"
#include "MainWindow.h"
#include "Include/IObjectManager.h"
#include "ActionManager.h"

// Including this too early will result in a linker error
#include <CryCommon/CryLibrary.h>


static const char defaultFileExtension[] = ".ly";
static const char oldFileExtension[] = ".cry";

// Implementation of System Callback structure.
struct SSystemUserCallback
    : public ISystemUserCallback
{
    SSystemUserCallback(IInitializeUIInfo* logo) : m_threadErrorHandler(this) { m_pLogo = logo; };
    virtual void OnSystemConnect(ISystem* pSystem)
    {
        ModuleInitISystem(pSystem, "Editor");
    }

    virtual bool OnError(const char* szErrorString)
    {
        // since we show a message box, we have to use the GUI thread
        if (QThread::currentThread() != qApp->thread())
        {
            bool result = false;
            QMetaObject::invokeMethod(&m_threadErrorHandler, "OnError", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result), Q_ARG(const char*, szErrorString));
            return result;
        }

        if (szErrorString)
        {
            Log(szErrorString);
        }

        if (GetIEditor()->IsInTestMode())
        {
            exit(1);
        }

        char str[4096];

        if (szErrorString)
        {
            azsnprintf(str, 4096, "%s\r\nSave Level Before Exiting the Editor?", szErrorString);
        }
        else
        {
            azsnprintf(str, 4096, "Unknown Error\r\nSave Level Before Exiting the Editor?");
        }

        int res = IDNO;

        ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_no_crash_dialog") : NULL;

        if (!pCVar || pCVar->GetIVal() == 0)
        {
            res = QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Engine Error"), str, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        }

        if (res == QMessageBox::Yes || res == QMessageBox::No)
        {
            if (res == QMessageBox::Yes)
            {
                if (GetIEditor()->SaveDocument())
                {
                    QMessageBox::information(QApplication::activeWindow(), QObject::tr("Save"), QObject::tr("Level has been successfully saved!\r\nPress Ok to terminate Editor."));
                }
            }
        }

        return true;
    }

    virtual bool OnSaveDocument()
    {
        bool success = false;

        if (GetIEditor())
        {
            // Turn off save backup as we force a backup before reaching this point
            bool prevSaveBackup = gSettings.bBackupOnSave;
            gSettings.bBackupOnSave = false;

            success = GetIEditor()->SaveDocument();
            gSettings.bBackupOnSave = prevSaveBackup;
        }

        return success;
    }

    virtual bool OnBackupDocument()
    {
        CCryEditDoc* level = GetIEditor() ? GetIEditor()->GetDocument() : nullptr;
        if (level)
        {
            return level->BackupBeforeSave(true);
        }

        return false;
    }

    virtual void OnProcessSwitch()
    {
        if (GetIEditor()->IsInGameMode())
        {
            GetIEditor()->SetInGameMode(false);
        }
    }

    virtual void OnInitProgress(const char* sProgressMsg)
    {
        if (m_pLogo)
        {
            m_pLogo->SetInfoText(sProgressMsg);
        }
    }

    virtual int ShowMessage(const char* text, const char* caption, unsigned int uType)
    {
        if (CCryEditApp::instance()->IsInAutotestMode())
        {
            return IDOK;
        }

        const UINT kMessageBoxButtonMask = 0x000f;
        if (!GetIEditor()->IsInGameMode() && (uType == 0 || uType == MB_OK || !(uType & kMessageBoxButtonMask)))
        {
            static_cast<CEditorImpl*>(GetIEditor())->AddErrorMessage(text, caption);
            return IDOK;
        }
        return CryMessageBox(text, caption, uType);
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer)
    {
        GetIEditor()->GetMemoryUsage(pSizer);
    }

    void OnSplashScreenDone()
    {
        m_pLogo = nullptr;
    }

private:
    IInitializeUIInfo* m_pLogo;
    ThreadedOnErrorHandler m_threadErrorHandler;
};

ThreadedOnErrorHandler::ThreadedOnErrorHandler(ISystemUserCallback* callback)
    : m_userCallback(callback)
{
    moveToThread(qApp->thread());
}

ThreadedOnErrorHandler::~ThreadedOnErrorHandler()
{
}

bool ThreadedOnErrorHandler::OnError(const char* error)
{
    return m_userCallback->OnError(error);
}

//! This class will be used by CSystem to find out whether the negotiation with the assetprocessor failed
class AssetProcessConnectionStatus
    : public AzFramework::AssetSystemConnectionNotificationsBus::Handler
{
public:
    AssetProcessConnectionStatus()
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusConnect();
    };
    ~AssetProcessConnectionStatus()
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusDisconnect();
    }

    //! Notifies listeners that connection to the Asset Processor failed
    void ConnectionFailed() override
    {
        m_connectionFailed = true;
    }

    void NegotiationFailed() override
    {
        m_negotiationFailed = true;
    }

    bool CheckConnectionFailed()
    {
        return m_connectionFailed;
    }

    bool CheckNegotiationFailed()
    {
        return m_negotiationFailed;
    }
private:
    bool m_connectionFailed = false;
    bool m_negotiationFailed = false;
};

AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
CGameEngine::CGameEngine()
    : m_gameDll(0)
    , m_bIgnoreUpdates(false)
    , m_ePendingGameMode(ePGM_NotPending)
    , m_modalWindowDismisser(nullptr)
AZ_POP_DISABLE_WARNING
{
    m_pISystem = NULL;
    m_bLevelLoaded = false;
    m_bInGameMode = false;
    m_bSimulationMode = false;
    m_bSyncPlayerPosition = true;
    m_hSystemHandle = 0;
    m_bJustCreated = false;
    m_levelName = "Untitled";
    m_levelExtension = defaultFileExtension;
    m_playerViewTM.SetIdentity();
    GetIEditor()->RegisterNotifyListener(this);
    AZ::Interface<IEditorCameraController>::Register(this);
}

AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
CGameEngine::~CGameEngine()
{
AZ_POP_DISABLE_WARNING
    AZ::Interface<IEditorCameraController>::Unregister(this);
    GetIEditor()->UnregisterNotifyListener(this);
    m_pISystem->GetIMovieSystem()->SetCallback(NULL);
    CEdMesh::ReleaseAll();

    if (m_gameDll)
    {
        CryFreeLibrary(m_gameDll);
    }

    delete m_pISystem;
    m_pISystem = NULL;

    if (m_hSystemHandle)
    {
        CryFreeLibrary(m_hSystemHandle);
    }

    delete m_pSystemUserCallback;
}

static int ed_killmemory_size;
static int ed_indexfiles;

void KillMemory(IConsoleCmdArgs* /* pArgs */)
{
    while (true)
    {
        const int kLimit = 10000000;
        int size;

        if (ed_killmemory_size > 0)
        {
            size = ed_killmemory_size;
        }
        else
        {
            size = rand() * rand();
            size = size > kLimit ? kLimit : size;
        }

        uint8* alloc = new uint8[size];
    }
}

static void CmdGotoEditor(IConsoleCmdArgs* pArgs)
{
    // feature is mostly useful for QA purposes, this works with the game "goto" command
    // this console command actually is used by the game command, the editor command shouldn't be used by the user
    int iArgCount = pArgs->GetArgCount();

    CViewManager* pViewManager = GetIEditor()->GetViewManager();
    CViewport* pRenderViewport = pViewManager->GetGameViewport();
    if (!pRenderViewport)
    {
        return;
    }

    float x, y, z, wx, wy, wz;

    if (iArgCount == 7
        && azsscanf(pArgs->GetArg(1), "%f", &x) == 1
        && azsscanf(pArgs->GetArg(2), "%f", &y) == 1
        && azsscanf(pArgs->GetArg(3), "%f", &z) == 1
        && azsscanf(pArgs->GetArg(4), "%f", &wx) == 1
        && azsscanf(pArgs->GetArg(5), "%f", &wy) == 1
        && azsscanf(pArgs->GetArg(6), "%f", &wz) == 1)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();

        tm.SetTranslation(Vec3(x, y, z));
        tm.SetRotation33(Matrix33::CreateRotationXYZ(DEG2RAD(Ang3(wx, wy, wz))));
        pRenderViewport->SetViewTM(tm);
    }
}

void CGameEngine::SetCurrentViewPosition(const AZ::Vector3& position)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        CUndo undo("Set Current View Position");
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoViewPosition());
        }
        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetTranslation(Vec3(position.GetX(), position.GetY(), position.GetZ()));
        pRenderViewport->SetViewTM(tm);
    }
}

void CGameEngine::SetCurrentViewRotation(const AZ::Vector3& rotation)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        CUndo undo("Set Current View Rotation");
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoViewRotation());
        }
        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetRotationXYZ(Ang3(DEG2RAD(rotation.GetX()), DEG2RAD(rotation.GetY()), DEG2RAD(rotation.GetZ())), tm.GetTranslation());
        pRenderViewport->SetViewTM(tm);
    }
}

AZ::Outcome<void, AZStd::string> CGameEngine::Init(
    bool bPreviewMode,
    bool bTestMode,
    bool bShaderCacheGen,
    const char* sInCmdLine,
    IInitializeUIInfo* logo,
    HWND hwndForInputSystem)
{
    m_pSystemUserCallback = new SSystemUserCallback(logo);
    m_hSystemHandle = CryLoadLibraryDefName("CrySystem");

    if (!m_hSystemHandle)
    {
        auto errorMessage = AZStd::string::format("%s Loading Failed", CryLibraryDefName("CrySystem"));
        Error(errorMessage.c_str());
        return AZ::Failure(errorMessage);
    }

    PFNCREATESYSTEMINTERFACE pfnCreateSystemInterface =
        (PFNCREATESYSTEMINTERFACE)CryGetProcAddress(m_hSystemHandle, "CreateSystemInterface");


    // Locate the root path
    const char* calcRootPath = nullptr;
    EBUS_EVENT_RESULT(calcRootPath, AZ::ComponentApplicationBus, GetAppRoot);
    if (calcRootPath == nullptr)
    {
        // If the app root isnt available, default to the engine root
        EBUS_EVENT_RESULT(calcRootPath, AzToolsFramework::ToolsApplicationRequestBus, GetEngineRootPath);
    }
    const char* searchPath[] = { calcRootPath };
    CEngineConfig engineConfig(searchPath,AZ_ARRAY_SIZE(searchPath)); // read the engine config also to see what game is running, and what folder(s) there are.

    SSystemInitParams sip;
    engineConfig.CopyToStartupParams(sip);

    sip.connectToRemote = true; // editor always connects
    sip.waitForConnection = true; // editor REQUIRES connect.
    const char localIP[10] = "127.0.0.1";
    azstrncpy(sip.remoteIP, AZ_ARRAY_SIZE(sip.remoteIP), localIP, AZ_ARRAY_SIZE(localIP)); // editor ONLY connects to the local asset processor

    sip.bEditor = true;
    sip.bDedicatedServer = false;
    sip.bPreview = bPreviewMode;
    sip.bTestMode = bTestMode;
    sip.hInstance = nullptr;

    sip.pSharedEnvironment = AZ::Environment::GetInstance();

#ifdef AZ_PLATFORM_MAC
    // Create a hidden QWidget. Would show a black window on macOS otherwise.
    auto window = new QWidget();
    QObject::connect(qApp, &QApplication::lastWindowClosed, window, &QWidget::deleteLater);
    sip.hWnd = (HWND)window->winId();
#else
    sip.hWnd = hwndForInputSystem;
#endif
    sip.hWndForInputSystem = hwndForInputSystem;

    sip.pLogCallback = &m_logFile;
    sip.sLogFileName = "@log@/Editor.log";
    sip.pUserCallback = m_pSystemUserCallback;
    sip.pValidator = GetIEditor()->GetErrorReport(); // Assign validator from Editor.

    // Calculate the branch token first based on the app root path if possible
    if (calcRootPath!=nullptr)
    {
        AZStd::string appRoot(calcRootPath);
        AZStd::string branchToken;
        AzFramework::StringFunc::AssetPath::CalculateBranchToken(appRoot, branchToken);
        azstrncpy(sip.branchToken, AZ_ARRAY_SIZE(sip.branchToken), branchToken.c_str(), branchToken.length());
    }

    if (sInCmdLine)
    {
        azstrncpy(sip.szSystemCmdLine, AZ_COMMAND_LINE_LEN, sInCmdLine, AZ_COMMAND_LINE_LEN);
        if (strstr(sInCmdLine, "-export") || strstr(sInCmdLine, "/export") || strstr(sInCmdLine, "-autotest_mode"))
        {
            sip.bUnattendedMode = true;
        }
    }

    if (sip.bUnattendedMode)
    {
        m_modalWindowDismisser = AZStd::make_unique<ModalWindowDismisser>();
    }

    if (bShaderCacheGen)
    {
        sip.bSkipFont = true;
    }
    AssetProcessConnectionStatus    apConnectionStatus;

    m_pISystem = pfnCreateSystemInterface(sip);

    if (!gEnv)
    {
        gEnv = m_pISystem->GetGlobalEnvironment();
    }

    if (!m_pISystem)
    {
        AZStd::string errorMessage = "Could not initialize CSystem.  View the logs for more details.";

        gEnv = nullptr;
        Error("CreateSystemInterface Failed");
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckNegotiationFailed())
    {
        auto errorMessage = AZStd::string::format("Negotiation with Asset Processor failed.\n"
            "Please ensure the Asset Processor is running on the same branch and try again.");
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckConnectionFailed())
    {
        auto errorMessage = AZStd::string::format("Unable to connect to the local Asset Processor.\n\n"
                                                  "The Asset Processor is either not running locally or not accepting connections on port %d. "
                                                  "Check your remote_port settings in bootstrap.cfg or view the Asset Processor's \"Logs\" tab "
                                                  "for any errors.", sip.remotePort);
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    // because we're the editor here, we also give tool aliases to the original, unaltered roots:
    string devAssetsFolder = engineConfig.m_rootFolder + "/" + engineConfig.m_gameFolder;
    if (gEnv && gEnv->pFileIO)
    {
        const char* engineRoot = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRoot, &AzToolsFramework::ToolsApplicationRequests::GetEngineRootPath);
        if (engineRoot != nullptr)
        {
            gEnv->pFileIO->SetAlias("@engroot@", engineRoot);
        }
        else
        {
            gEnv->pFileIO->SetAlias("@engroot@", engineConfig.m_rootFolder.c_str());
        }

        gEnv->pFileIO->SetAlias("@devroot@", engineConfig.m_rootFolder.c_str());
        gEnv->pFileIO->SetAlias("@devassets@", devAssetsFolder.c_str());
    }

    SetEditorCoreEnvironment(gEnv);

    if (gEnv
        && gEnv->p3DEngine
        && gEnv->p3DEngine->GetTimeOfDay())
    {
        gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();
    }

    if (gEnv && gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->EnablePhysicsEvents(m_bSimulationMode);
    }

    CLogFile::AboutSystem();
    REGISTER_CVAR(ed_killmemory_size, -1, VF_DUMPTODISK, "Sets the testing allocation size. -1 for random");
    REGISTER_CVAR(ed_indexfiles, 1, VF_DUMPTODISK, "Index game resource files, 0 - inactive, 1 - active");
    REGISTER_COMMAND("ed_killmemory", KillMemory, VF_NULL, "");
    REGISTER_COMMAND("ed_goto", CmdGotoEditor, VF_CHEAT, "Internal command, used by the 'GOTO' console command\n");

    // The editor needs to handle the quit command differently
    gEnv->pConsole->RemoveCommand("quit");
    REGISTER_COMMAND("quit", CGameEngine::HandleQuitRequest, VF_RESTRICTEDMODE, "Quit/Shutdown the engine");

    EBUS_EVENT(CrySystemEventBus, OnCryEditorInitialized);
    
    return AZ::Success();
}

bool CGameEngine::InitGame(const char*)
{
    // in editor we do it later, bExecuteCommandLine was set to false
    m_pISystem->ExecuteCommandLine();

    return true;
}

void CGameEngine::SetLevelPath(const QString& path)
{
    QByteArray levelPath;
    levelPath.reserve(AZ_MAX_PATH_LEN);
    levelPath = Path::ToUnixPath(Path::RemoveBackslash(path)).toUtf8();
    m_levelPath = levelPath;

    m_levelName = m_levelPath.mid(m_levelPath.lastIndexOf('/') + 1);

    // Store off if 
    if (QFileInfo(path + oldFileExtension).exists())
    {
        m_levelExtension = oldFileExtension;
    }
    else
    {
        m_levelExtension = defaultFileExtension;
    }

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->SetLevelPath(m_levelPath.toUtf8().data());
    }
}

void CGameEngine::SetMissionName(const QString& mission)
{
    m_missionName = mission;
}

bool CGameEngine::LoadLevel(
    const QString& mission,
    [[maybe_unused]] bool bDeleteAIGraph,
    bool bReleaseResources)
{
    LOADING_TIME_PROFILE_SECTION(GetIEditor()->GetSystem());
    m_bLevelLoaded = false;
    m_missionName = mission;
    CLogFile::FormatLine("Loading map '%s' into engine...", m_levelPath.toUtf8().data());
    // Switch the current directory back to the Primary CD folder first.
    // The engine might have trouble to find some files when the current
    // directory is wrong
    QDir::setCurrent(GetIEditor()->GetPrimaryCDFolder());

    QString pakFile = m_levelPath + "/level.pak";

    // Open Pak file for this level.
    if (!m_pISystem->GetIPak()->OpenPack(m_levelPath.toUtf8().data(), pakFile.toUtf8().data()))
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Level Pack File %s Not Found", pakFile.toUtf8().data());
    }

    // Initialize physics grid.
    if (bReleaseResources)
    {
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        int physicsEntityGridSize = static_cast<int>(terrainAabb.GetXExtent());

        //CryPhysics under performs if physicsEntityGridSize < nTerrainSize.
        if (physicsEntityGridSize <= 0)
        {
            ICVar* pCvar = m_pISystem->GetIConsole()->GetCVar("e_PhysEntityGridSizeDefault");
            AZ_Assert(pCvar, "The CVAR e_PhysEntityGridSizeDefault is not defined");
            physicsEntityGridSize = pCvar->GetIVal();
        }

    }

    // Load level in 3d engine.
    if (!gEnv->p3DEngine->InitLevelForEditor(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data()))
    {
        CLogFile::WriteLine("ERROR: Can't load level !");
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("ERROR: Can't load level !"));
        return false;
    }

    // Audio: notify audio of level loading start?
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_REFRESH);

    m_bLevelLoaded = true;

    if (!bReleaseResources)
    {
        ReloadEnvironment();
    }

    return true;
}

bool CGameEngine::ReloadLevel()
{
    if (!LoadLevel(GetMissionName(), false, false))
    {
        return false;
    }

    return true;
}

bool CGameEngine::LoadMission(const QString& mission)
{
    if (!IsLevelLoaded())
    {
        return false;
    }

    if (mission != m_missionName)
    {
        m_missionName = mission;
        gEnv->p3DEngine->LoadMissionDataFromXMLNode(m_missionName.toUtf8().data());
    }

    return true;
}

bool CGameEngine::ReloadEnvironment()
{
    if (!gEnv->p3DEngine)
    {
        return false;
    }

    if (!IsLevelLoaded() && !m_bJustCreated)
    {
        return false;
    }

    if (!GetIEditor()->GetDocument())
    {
        return false;
    }

    XmlNodeRef env = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(GetIEditor()->GetDocument()->GetEnvironmentTemplate(), env);

    // Notify mission that environment may be changed.
    GetIEditor()->GetDocument()->GetCurrentMission()->OnEnvironmentChange();

    QString xmlStr = QString::fromLatin1(env->getXML());

    // Reload level data in engine.
    gEnv->p3DEngine->LoadEnvironmentSettingsFromXML(env);

    return true;
}

void CGameEngine::SwitchToInGame()
{
    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->DisablePostEffects();
        gEnv->p3DEngine->ResetPostEffects();
    }

    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
    if (streamer)
    {
        AZStd::binary_semaphore wait;
        AZ::IO::FileRequestPtr flush = streamer->FlushCaches();
        streamer->SetRequestCompleteCallback(flush, [&wait](AZ::IO::FileRequestHandle) { wait.release(); });
        streamer->QueueRequest(flush);
        wait.acquire();
    }
    
    GetIEditor()->Notify(eNotify_OnBeginGameMode);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->ResetParticlesAndDecals();
    }

    CViewport* pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(true);
    m_bInGameMode = true;

    CRuler* pRuler = GetIEditor()->GetRuler();
    if (pRuler)
    {
        pRuler->SetActive(false);
    }

    gEnv->p3DEngine->GetTimeOfDay()->EndEditMode();
    gEnv->pSystem->GetViewCamera().SetMatrix(m_playerViewTM);

    // Disable accelerators.
    GetIEditor()->EnableAcceleratos(false);
    //! Send event to switch into game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);

    m_pISystem->GetIMovieSystem()->Reset(true, false);

    // Transition to runtime entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StartPlayInEditor);

    if (!CCryEditApp::instance()->IsInAutotestMode())
    {
        // Constrain and hide the system cursor (important to do this last)
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::ConstrainedAndHidden);
    }
}

void CGameEngine::SwitchToInEditor()
{
    // Transition to editor entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StopPlayInEditor);

    // Reset movie system
    for (int i = m_pISystem->GetIMovieSystem()->GetNumPlayingSequences(); --i >= 0;)
    {
        m_pISystem->GetIMovieSystem()->GetPlayingSequence(i)->Deactivate();
    }
    m_pISystem->GetIMovieSystem()->Reset(false, false);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (gEnv->p3DEngine)
    {
        // Reset 3d engine effects
        gEnv->p3DEngine->DisablePostEffects();
        gEnv->p3DEngine->ResetPostEffects();
        gEnv->p3DEngine->ResetParticlesAndDecals();
    }

    CViewport* pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(m_bSimulationMode);
    gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();

    // this has to be done before the RemoveSink() call, or else some entities may not be removed
    gEnv->p3DEngine->GetDeferredPhysicsEventManager()->ClearDeferredEvents();

    // Enable accelerators.
    GetIEditor()->EnableAcceleratos(true);


    // reset UI system
    if (gEnv->pLyShine)
    {
        gEnv->pLyShine->Reset();
    }

    // [Anton] - order changed, see comments for CGameEngine::SetSimulationMode
    //! Send event to switch out of game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);

    m_bInGameMode = false;

    // save the current gameView matrix for editor
    if (pGameViewport)
    {
        Matrix34 gameView = gEnv->pSystem->GetViewCamera().GetMatrix();
        pGameViewport->SetGameTM(gameView);
    }

    // Out of game in Editor mode.
    if (pGameViewport)
    {
        pGameViewport->SetViewTM(m_playerViewTM);
    }


    GetIEditor()->Notify(eNotify_OnEndGameMode);

    // Unconstrain the system cursor and make it visible (important to do this last)
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::UnconstrainedAndVisible);
}

void CGameEngine::HandleQuitRequest(IConsoleCmdArgs* /*args*/)
{
    if (GetIEditor()->GetGameEngine()->IsInGameMode())
    {
        GetIEditor()->GetGameEngine()->RequestSetGameMode(false);
        gEnv->pConsole->ShowConsole(false);
    }
    else
    {
        MainWindow::instance()->GetActionManager()->GetAction(ID_APP_EXIT)->trigger();
    }
}

void CGameEngine::RequestSetGameMode(bool inGame)
{
    m_ePendingGameMode = inGame ? ePGM_SwitchToInGame : ePGM_SwitchToInEditor;

    if (m_ePendingGameMode == ePGM_SwitchToInGame)
    {
        AzToolsFramework::EditorLegacyGameModeNotificationBus::Broadcast(
            &AzToolsFramework::EditorLegacyGameModeNotificationBus::Events::OnStartGameModeRequest);
    }
    else if (m_ePendingGameMode == ePGM_SwitchToInEditor)
    {
        AzToolsFramework::EditorLegacyGameModeNotificationBus::Broadcast(
            &AzToolsFramework::EditorLegacyGameModeNotificationBus::Events::OnStopGameModeRequest);
    }
}

void CGameEngine::SetGameMode(bool bInGame)
{
    if (m_bInGameMode == bInGame)
    {
        return;
    }

    if (!GetIEditor()->GetDocument())
    {
        return;
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_START, bInGame, 0);

    // Enables engine to know about that.
    gEnv->SetIsEditorGameMode(bInGame);

    // Ignore updates while changing in and out of game mode
    m_bIgnoreUpdates = true;
    LockResources();

    // Switching modes will destroy the current AzFramework::EntityConext which may contain
    // data the queued events hold on to, so execute all queued events before switching.
    ExecuteQueuedEvents();

    if (bInGame)
    {
        SwitchToInGame();
    }
    else
    {
        SwitchToInEditor();
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Enables engine to know about that.
    if (MainWindow::instance())
    {
        AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
        MainWindow::instance()->setFocus();
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED, bInGame, 0);

    UnlockResources();
    m_bIgnoreUpdates = false;

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_END, bInGame, 0);
}

void CGameEngine::SetSimulationMode(bool enabled, bool bOnlyPhysics)
{
    if (m_bSimulationMode == enabled)
    {
        return;
    }

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(enabled);

    if (!bOnlyPhysics)
    {
        LockResources();
    }

    if (enabled)
    {
        CRuler* pRuler = GetIEditor()->GetRuler();
        if (pRuler)
        {
            pRuler->SetActive(false);
        }

        GetIEditor()->Notify(eNotify_OnBeginSimulationMode);
    }
    else
    {
        GetIEditor()->Notify(eNotify_OnEndSimulationMode);
    }

    m_bSimulationMode = enabled;

    // Enables engine to know about simulation mode.
    gEnv->SetIsEditorSimulationMode(enabled);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (m_bSimulationMode)
    {
        if (!bOnlyPhysics)
        {
            if (m_pISystem->GetI3DEngine())
            {
                m_pISystem->GetI3DEngine()->ResetPostEffects();
            }

            GetIEditor()->SetConsoleVar("ai_ignoreplayer", 1);
            //GetIEditor()->SetConsoleVar( "ai_soundperception",0 );
        }

        // [Anton] the order of the next 3 calls changed, since, EVENT_INGAME loads physics state (if any),
        // and Reset should be called before it
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);
    }
    else
    {
        if (!bOnlyPhysics)
        {
            GetIEditor()->SetConsoleVar("ai_ignoreplayer", 0);
            //GetIEditor()->SetConsoleVar( "ai_soundperception",1 );

            if (m_pISystem->GetI3DEngine())
            {
                m_pISystem->GetI3DEngine()->ResetPostEffects();
            }
        }


        GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Execute all queued events before switching modes.
    ExecuteQueuedEvents();

    // Transition back to editor entity context.
    // Symmetry is not critical. It's okay to call this even if we never called StartPlayInEditor
    // (bOnlyPhysics was true when we entered simulation mode).
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StopPlayInEditor);

    if (m_bSimulationMode && !bOnlyPhysics)
    {
        // Transition to runtime entity context.
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StartPlayInEditor);
    }

    if (!bOnlyPhysics)
    {
        UnlockResources();
    }

    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
}

void CGameEngine::ResetResources()
{
    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->UnloadLevel();
    }

}

void CGameEngine::SetPlayerViewMatrix(const Matrix34& tm, [[maybe_unused]] bool bEyePos)
{
    m_playerViewTM = tm;
}

void CGameEngine::SyncPlayerPosition(bool bEnable)
{
    m_bSyncPlayerPosition = bEnable;

    if (m_bSyncPlayerPosition)
    {
        SetPlayerViewMatrix(m_playerViewTM);
    }
}

void CGameEngine::SetCurrentMOD(const char* sMod)
{
    m_MOD = sMod;
}

QString CGameEngine::GetCurrentMOD() const
{
    return m_MOD;
}

void CGameEngine::Update()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    switch (m_ePendingGameMode)
    {
    case ePGM_SwitchToInGame:
    {
        SetGameMode(true);
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }

    case ePGM_SwitchToInEditor:
    {
        bool wasInSimulationMode = GetIEditor()->GetGameEngine()->GetSimulationMode();
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(false);
        }
        SetGameMode(false);
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(true);
        }
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }
    }

    AZ::ComponentApplication* componentApplication = nullptr;
    EBUS_EVENT_RESULT(componentApplication, AZ::ComponentApplicationBus, GetApplication);

    if (m_bInGameMode)
    {
        CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();

        // if we're in editor mode, match the width, height and Fov, but alter no other parameters
        if (pRenderViewport)
        {
            int width = 640;
            int height = 480;
            pRenderViewport->GetDimensions(&width, &height);

            // Check for custom width and height cvars in use by Track View.
            // The backbuffer size maybe have been changed, so we need to make sure the viewport
            // is setup with the correct aspect ratio here so the captured output will look correct.
            ICVar* cVar = gEnv->pConsole->GetCVar("TrackViewRenderOutputCapturing");
            if (cVar && cVar->GetIVal() != 0)
            {
                const int customWidth = gEnv->pConsole->GetCVar("r_CustomResWidth")->GetIVal();
                const int customHeight = gEnv->pConsole->GetCVar("r_CustomResHeight")->GetIVal();
                if (customWidth != 0 && customHeight != 0)
                {
                    IEditor* editor = GetIEditor();
                    AZ_Assert(editor, "Expected valid Editor");
                    IRenderer* renderer = editor->GetRenderer();
                    AZ_Assert(renderer, "Expected valid Renderer");

                    int maxRes = renderer->GetMaxSquareRasterDimension();
                    width = clamp_tpl(customWidth, 32, maxRes);
                    height = clamp_tpl(customHeight, 32, maxRes);
                }
            }

            CCamera& cam = gEnv->pSystem->GetViewCamera();
            cam.SetFrustum(width, height, pRenderViewport->GetFOV(), cam.GetNearPlane(), cam.GetFarPlane(), cam.GetPixelAspectRatio());
        }

        if (gEnv->pSystem)
        {
            gEnv->pSystem->UpdatePreTickBus();
            componentApplication->Tick(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_GAME));
            gEnv->pSystem->UpdatePostTickBus();
        }

        // TODO: still necessary after AVI recording removal?
        if (pRenderViewport)
        {
            // Make sure we at least try to update game viewport (Needed for AVI recording).
            pRenderViewport->Update();
        }
    }
    else
    {
        // [marco] check current sound and vis areas for music etc.
        // but if in game mode, 'cos is already done in the above call to game->update()
        unsigned int updateFlags = ESYSUPDATE_EDITOR;

        CRuler* pRuler = GetIEditor()->GetRuler();
        const bool bRulerNeedsUpdate = (pRuler && pRuler->HasQueuedPaths());

        if (!m_bSimulationMode)
        {
            updateFlags |= ESYSUPDATE_IGNORE_PHYSICS;
        }

        bool bUpdateAIPhysics = GetSimulationMode();

        if (bUpdateAIPhysics)
        {
            updateFlags |= ESYSUPDATE_EDITOR_AI_PHYSICS;
        }

        GetIEditor()->GetAnimation()->Update();
        GetIEditor()->GetSystem()->UpdatePreTickBus(updateFlags);
        componentApplication->Tick(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_GAME));
        GetIEditor()->GetSystem()->UpdatePostTickBus(updateFlags);
    }
}

void CGameEngine::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginSceneOpen:
    {
        ResetResources();
    }
    break;
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndTerrainRebuild:
    {
    }
    case eNotify_OnEndNewScene: // intentional fall-through?
    {
        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->PostLoadLevel();
        }
    }
    break;
    case eNotify_OnSplashScreenDestroyed:
    {
        if (m_pSystemUserCallback != NULL)
        {
            m_pSystemUserCallback->OnSplashScreenDone();
        }
    }
    break;
    }
}

void CGameEngine::LockResources()
{
    gEnv->p3DEngine->LockCGFResources();
}

void CGameEngine::UnlockResources()
{
    gEnv->p3DEngine->UnlockCGFResources();
}

void CGameEngine::OnTerrainModified(const Vec2& modPosition, float modAreaRadius, bool fullTerrain)
{
    INavigationSystem* pNavigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)

    if (pNavigationSystem)
    {
        // Only report local modifications, not a change in the full terrain (probably happening during initialization)
        if (fullTerrain == false)
        {
            const Vec2 offset(modAreaRadius * 1.5f, modAreaRadius * 1.5f);
            AABB updateBox;
            updateBox.min = modPosition - offset;
            updateBox.max = modPosition + offset;
            AzFramework::Terrain::TerrainDataRequests* terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
            AZ_Assert(terrain != nullptr, "Expecting a valid terrain handler when the terrain is modified");
            const float terrainHeight1 = terrain->GetHeightFromFloats(updateBox.min.x, updateBox.min.y);
            const float terrainHeight2 = terrain->GetHeightFromFloats(updateBox.max.x, updateBox.max.y);
            const float terrainHeight3 = terrain->GetHeightFromFloats(modPosition.x, modPosition.y);

            updateBox.min.z = min(terrainHeight1, min(terrainHeight2, terrainHeight3)) - (modAreaRadius * 2.0f);
            updateBox.max.z = max(terrainHeight1, max(terrainHeight2, terrainHeight3)) + (modAreaRadius * 2.0f);
            pNavigationSystem->WorldChanged(updateBox);
        }
    }
}

void CGameEngine::OnAreaModified(const AABB& modifiedArea)
{
    INavigationSystem* pNavigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
    if (pNavigationSystem)
    {
        pNavigationSystem->WorldChanged(modifiedArea);
    }
}

void CGameEngine::ExecuteQueuedEvents()
{
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();
    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();
}

#include <moc_GameEngine.cpp>
