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

#pragma once
#include <Urho3D/Core/Object.h>

using namespace Urho3D;
namespace Urho3D
{
class Scene;
}

class Lightmap;

//=============================================================================
//=============================================================================
URHO3D_EVENT(E_LIGHTMAPSTATUS, LightmapStatus)
{
    URHO3D_PARAM(P_TOTAL, TotalCnt);        // total count
    URHO3D_PARAM(P_COMPLETED, CompleteCnt); // complete count
}

//=============================================================================
//=============================================================================
class LightmapCreator : public Object
{
    URHO3D_OBJECT(LightmapCreator, Object);

public:
    LightmapCreator(Context* context);
    virtual ~LightmapCreator();

    void Init(Scene *scene, const String& outputPath);
    void GenerateLightmaps();

protected:
    unsigned ParseModelsInScene();
    void QueueNodeProcess();
    void StartLightmapBuild(Node *node);
    void RemoveCompletedNode(Node *node);
    void SendEventMsg();
    void HandleBuildEvent(StringHash eventType, VariantMap& eventData);

protected:
    WeakPtr<Scene> scene_;
    String outputPath_;

    PODVector<Node*> buildRequiredNodeList_;
    PODVector<Node*> origNodeList_;
    PODVector<Node*> processingNodeList_;

    unsigned totalCnt_;
    unsigned numProcessed_;
    unsigned maxThreads_;
};


