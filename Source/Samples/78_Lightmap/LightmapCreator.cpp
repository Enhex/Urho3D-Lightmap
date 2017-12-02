//
// Copyright (c) 2008-2017 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include "LightmapCreator.h"
#include "Lightmap.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
LightmapCreator::LightmapCreator(Context* context)
    : Object(context)
    , totalCnt_(0)
    , numProcessed_(0)
    , maxThreads_(8)
    , lightmapState_(LightMap_UnInit)
{
    Lightmap::RegisterObject(context);
}

LightmapCreator::~LightmapCreator()
{
}

void LightmapCreator::Init(Scene *scene, const String& outputPath)
{
    scene_ = scene;
    outputPath_ = outputPath;
}

void LightmapCreator::GenerateLightmaps()
{
    // change fog color
    Zone *zone = scene_->GetComponent<Zone>();
    if (zone)
    {
        origFogColor_ = zone->GetFogColor();
        zone->SetFogColor(Color::BLACK);
    }

    // 1st step
    lightmapState_ = LightMap_DirectLight;

    ParseModelsInScene();
    QueueNodesForDirectLightBaking();

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(LightmapCreator, HandleUpdate));
    SubscribeToEvent(E_DIRECTLIGHTINGDONE, URHO3D_HANDLER(LightmapCreator, HandleDirectLightBuildEvent));
}

void LightmapCreator::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    switch (lightmapState_)
    {
    case LightMap_DirectLight:
        if (numProcessed_ == totalCnt_)
        {
            lightmapState_ = LightMap_IndirectLightBegin;
        }
        break;

    case LightMap_IndirectLightBegin:
        {
            SetupIndirectProcess();

            // state chng
            lightmapState_ = LightMap_IndirectLightProcessing;
        }
        break;

    case LightMap_IndirectLightProcessing:
        if (numObjectsCompletedIndirect_ == origNodeList_.Size())
        {
            // have main clear status msg
            SendIndirectLightingStatus(true);

            lightmapState_ = LightMap_SwapToLightmapTexture;
        }
        break;

    case LightMap_SwapToLightmapTexture:
        {
            for ( unsigned i = 0; i < origNodeList_.Size(); ++i )
            {
                Lightmap *lightmap = origNodeList_[i]->GetComponent<Lightmap>();
                lightmap->SwatToLightmapTechnique();
            }
            lightmapState_ = LightMap_RestoreScene;
        }
        break;

    case LightMap_DilateTextures:
        break;

    case LightMap_RestoreScene:
        {
            Zone *zone = scene_->GetComponent<Zone>();
            if (zone)
            {
                zone->SetFogColor(origFogColor_);
            }

            lightmapState_ = LightMap_Complete;
        }
        break;
    }
}

unsigned LightmapCreator::ParseModelsInScene()
{
    PODVector<Node*> result;
    scene_->GetChildrenWithComponent(result, "StaticModel", true);

    for ( unsigned i = 0; i < result.Size(); ++i )
    {
        // texcoord2 is required
        if (HasTexCoord2(result[i]->GetComponent<StaticModel>()))
        {
            // create lightmap component and init model settings required for baking process
            Lightmap *lightmap = result[i]->CreateComponent<Lightmap>();
            lightmap->InitModelSetting(ViewMask_Default);
            lightmap->SetSavefile(false);

            origNodeList_.Push(result[i]);
            buildRequiredNodeList_.Push(result[i]);
        }
    }

    totalCnt_ = origNodeList_.Size();

    return totalCnt_;
}

bool LightmapCreator::HasTexCoord2(StaticModel *staticModel)
{
    bool hasTexCoord2 = false;

    if (staticModel)
    {
        Model *model = staticModel->GetModel();
        Geometry *geometry = model->GetGeometry(0, 0);
        VertexBuffer *vbuffer = geometry->GetVertexBuffer(0);

        hasTexCoord2 = (vbuffer->GetElementMask() & MASK_TEXCOORD2) != 0;
    }

    return hasTexCoord2;
}

void LightmapCreator::SetupIndirectProcess()
{
    // clear vars
    totalTriangleCnt_ = 0;
    trianglesCompleted_ = 0;
    numObjectsCompletedIndirect_ = 0;

    SubscribeToEvent(E_TRIANGLEINFO, URHO3D_HANDLER(LightmapCreator, HandleTriangleInfoEvent));
    SubscribeToEvent(E_TRIANGLECOMPLETED, URHO3D_HANDLER(LightmapCreator, HandleTriangleCompletedEvent));
    SubscribeToEvent(E_INDIRECTCOMPLETED, URHO3D_HANDLER(LightmapCreator, HandleIndirectCompletedEvent));

    buildRequiredNodeList_.Resize(origNodeList_.Size());
    processingNodeList_.Clear();

    for ( unsigned i = 0; i < origNodeList_.Size(); ++i )
    {
        buildRequiredNodeList_[i] = origNodeList_[i];
    }

    QueueNodesForIndirectLightProcess();

}

void LightmapCreator::QueueNodesForDirectLightBaking()
{
    // only bake one direct light at a time
    while (buildRequiredNodeList_.Size() && processingNodeList_.Size() < 1)
    {
        Node* node = buildRequiredNodeList_[0];

        BakeDirectLight(node);

        processingNodeList_.Push(node);
        buildRequiredNodeList_.Erase(0);
    }
}

void LightmapCreator::QueueNodesForIndirectLightProcess()
{
    // only bake one direct light at a time
    while (buildRequiredNodeList_.Size() && processingNodeList_.Size() < maxThreads_)
    {
        Node* node = buildRequiredNodeList_[0];

        Lightmap *lightmap = node->GetComponent<Lightmap>();
        lightmap->BeginIndirectLighting(outputPath_);

        processingNodeList_.Push(node);
        buildRequiredNodeList_.Erase(0);
    }
}

void LightmapCreator::BakeDirectLight(Node *node)
{
    Lightmap *lightmap = node->GetComponent<Lightmap>();
    lightmap->BakeDirectLight(outputPath_);
}

void LightmapCreator::RemoveCompletedNode(Node *node)
{
    if (processingNodeList_.Remove(node))
    {
        ++numProcessed_;
    }

    // send event
    SendEventMsg();

    if (numProcessed_ != totalCnt_)
    {
        QueueNodesForDirectLightBaking();
    }
    else
    {
        RestoreModelSettigs();
    }
}

void LightmapCreator::RestoreModelSettigs()
{
    for ( unsigned i = 0; i < origNodeList_.Size(); ++i )
    {
        Lightmap *lightmap = origNodeList_[i]->GetComponent<Lightmap>();
        lightmap->RestoreModelSetting();
    }
}

void LightmapCreator::SendEventMsg()
{
    using namespace LightmapStatus;

    VariantMap& eventData  = GetEventDataMap();
    eventData[P_TOTAL]     = totalCnt_;
    eventData[P_COMPLETED] = numProcessed_;

    SendEvent(E_LIGHTMAPSTATUS, eventData);
}

void LightmapCreator::HandleDirectLightBuildEvent(StringHash eventType, VariantMap& eventData)
{
    using namespace DirectLightmapDone;
    Node *node = (Node*)eventData[P_NODE].GetVoidPtr();

    RemoveCompletedNode(node);
}

void LightmapCreator::SendIndirectLightingStatus(bool removemsg)
{
    using namespace IndirectLightStatus;

    VariantMap& eventData  = GetEventDataMap();
    eventData[P_TITLE]     = String("Indirect light tris: ");
    eventData[P_TOTAL]     = totalTriangleCnt_;
    eventData[P_COMPLETED] = trianglesCompleted_;
    eventData[P_REMOVEMSG] = removemsg;

    SendEvent(E_INDIRECTLIGHTSTATUS, eventData);
}

void LightmapCreator::HandleTriangleInfoEvent(StringHash eventType, VariantMap& eventData)
{
    using namespace TriangleInfo;
    totalTriangleCnt_ += eventData[P_TRICNT].GetUInt();

    SendIndirectLightingStatus();
}

void LightmapCreator::HandleTriangleCompletedEvent(StringHash eventType, VariantMap& eventData)
{
    ++trianglesCompleted_;

    SendIndirectLightingStatus();
}

void LightmapCreator::HandleIndirectCompletedEvent(StringHash eventType, VariantMap& eventData)
{
    using namespace IndirectCompleted;
    Node *node = (Node*)eventData[P_NODE].GetVoidPtr();

    ++numObjectsCompletedIndirect_;

    processingNodeList_.Remove(node);
    QueueNodesForIndirectLightProcess();
}

