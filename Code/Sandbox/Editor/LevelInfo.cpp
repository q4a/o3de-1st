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

#include "LevelInfo.h"

// Qt
#include <QMessageBox>

// Editor
#include "Util/fastlib.h"
#include "Material/MaterialManager.h"   // for CMaterialManager

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include "QtUI/WaitCursor.h"            // for WaitCursor
#include "Include/IErrorReport.h"
#include "Include/IObjectManager.h"
#include "Objects/BaseObject.h"
#include "UsedResources.h"
#include "ErrorReport.h"


//////////////////////////////////////////////////////////////////////////
CLevelInfo::CLevelInfo()
{
    m_pReport = GetIEditor()->GetErrorReport();
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::SaveLevelResources([[maybe_unused]] const QString& toPath)
{
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::Validate()
{
    m_pReport->Clear();
    m_pReport->SetImmediateMode(false);
    m_pReport->SetShowErrors(true);

    int nTotalErrors(0);
    int nCurrentError(0);

    // Here we are appending the current level load errors to the general errors.
    // Actually we are inserting them before all others, but this is not important :-).
    IErrorReport*   poLastLoadedLevelErrorReport = GetIEditor()->GetLastLoadedLevelErrorReport();
    if (poLastLoadedLevelErrorReport)
    {
        nTotalErrors = poLastLoadedLevelErrorReport->GetErrorCount();
        for (nCurrentError = 0; nCurrentError < nTotalErrors; ++nCurrentError)
        {
            m_pReport->ReportError(poLastLoadedLevelErrorReport->GetError(nCurrentError));
        }
    }

    // Validate level.
    ValidateObjects();
    ValidateMaterials();

    if (m_pReport->GetErrorCount() == 0)
    {
        QMessageBox::information(QApplication::activeWindow(), QString(), QObject::tr("No Errors Found"));
    }
    else
    {
        m_pReport->Display();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::ValidateObjects()
{
    WaitCursor cursor;

    // Validate all objects
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    int i;

    CLogFile::WriteLine("Validating Objects...");
    for (i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];

        m_pReport->SetCurrentValidatorObject(pObject);

        pObject->Validate(m_pReport);

        CUsedResources rs;
        pObject->GatherUsedResources(rs);
        rs.Validate(m_pReport);

        m_pReport->SetCurrentValidatorObject(NULL);
    }

    CLogFile::WriteLine("Validating Duplicate Objects...");
    //////////////////////////////////////////////////////////////////////////
    // Find duplicate objects, Same objects with same transform.
    // Use simple grid to speed up the check.
    //////////////////////////////////////////////////////////////////////////
    int gridSize = 256;

    AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
    float worldSize = terrainAabb.GetXExtent();

    float fGridToWorld = worldSize / gridSize;

    // Put all objects into parition grid.
    std::vector<std::list<CBaseObject*> > grid;
    grid.resize(gridSize * gridSize);
    // Put objects to grid.
    for (i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        Vec3 pos = pObject->GetWorldPos();
        int px = ftoi(pos.x / fGridToWorld);
        int py = ftoi(pos.y / fGridToWorld);
        if (px < 0)
        {
            px = 0;
        }
        if (py < 0)
        {
            py = 0;
        }
        if (px >= gridSize)
        {
            px = gridSize - 1;
        }
        if (py >= gridSize)
        {
            py = gridSize - 1;
        }
        grid[py * gridSize + px].push_back(pObject);
    }

    std::list<CBaseObject*>::iterator it1, it2;
    // Check objects in grid.
    for (i = 0; i < gridSize * gridSize; i++)
    {
        std::list<CBaseObject*>::iterator first = grid[i].begin();
        std::list<CBaseObject*>::iterator last = grid[i].end();
        for (it1 = first; it1 != last; ++it1)
        {
            for (it2 = first; it2 != it1; ++it2)
            {
                // Check if same object.
                CBaseObject* p1 = *it1;
                CBaseObject* p2 = *it2;
                if (p1 != p2 && p1->GetClassDesc() == p2->GetClassDesc())
                {
                    // Same class.
                    Quat q1 = p1->GetRotation();
                    Quat q2 = p2->GetRotation();
                    if (p1->GetWorldPos() == p2->GetWorldPos() && q1.w == q2.w && IsEquivalent(q1.v, q2.v, 0) && p1->GetScale() == p2->GetScale())
                    {
                        // Same transformation
                        // Check if objects are really same.
                        if (p1->IsSimilarObject(p2))
                        {
                            // Report duplicate objects.
                            CErrorRecord err;
                            err.error = QObject::tr("Found multiple objects in the same location (class %1): %2 and %3")
                                .arg(p1->GetClassDesc()->ClassName(), p1->GetName(), p2->GetName());
                            err.pObject = p1;
                            err.severity = CErrorRecord::ESEVERITY_ERROR;
                            m_pReport->ReportError(err);
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::ValidateMaterials()
{
    // Validate all objects
    CBaseObjectsArray objects;
    GetIEditor()->GetMaterialManager()->Validate();
}
