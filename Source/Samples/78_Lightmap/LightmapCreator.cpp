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
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
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
    , maxThreads_(1)
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

    SubscribeToEvent(E_LIGTHMAPDONE, URHO3D_HANDLER(LightmapCreator, HandleBuildEvent));
}

void LightmapCreator::GenerateLightmaps()
{
    ParseModelsInScene();
    QueueNodeProcess();
}

unsigned LightmapCreator::ParseModelsInScene()
{
    PODVector<Node*> result;
    scene_->GetChildrenWithComponent(result, "StaticModel", true);

    for ( unsigned i = 0; i < result.Size(); ++i )
    {
        origNodeList_.Push(result[i]);
        buildRequiredNodeList_.Push(result[i]);
    }
    totalCnt_ = origNodeList_.Size();

    return totalCnt_;
}

void LightmapCreator::QueueNodeProcess()
{
    while (buildRequiredNodeList_.Size() && processingNodeList_.Size() < maxThreads_)
    {
        Node* node = buildRequiredNodeList_[0];

        StartLightmapBuild(node);

        processingNodeList_.Push(node);
        buildRequiredNodeList_.Erase(0);
    }
}

void LightmapCreator::StartLightmapBuild(Node *node)
{
    Lightmap *lightmap = node->CreateComponent<Lightmap>();
    lightmap->BakeTexture(outputPath_);
}

void LightmapCreator::RemoveCompletedNode(Node *node)
{
    if (processingNodeList_.Remove(node))
    {
        node->RemoveComponent<Lightmap>();
        ++numProcessed_;
    }

    // send event
    SendEventMsg();

    if (numProcessed_ != totalCnt_)
    {
        QueueNodeProcess();
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

void LightmapCreator::HandleBuildEvent(StringHash eventType, VariantMap& eventData)
{
    using namespace LightmapDone;
    Node *node = (Node*)eventData[P_NODE].GetVoidPtr();

    RemoveCompletedNode(node);
}
