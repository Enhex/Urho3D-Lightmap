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

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;
namespace Urho3D
{
class Node;
class Texture2D;
class Camera;
class RenderSurface;
class BoundingBox;
class Material;
class StaticModel;
class Viewport;
}

//=============================================================================
//=============================================================================
URHO3D_EVENT(E_LIGTHMAPDONE, LightmapDone)
{
    URHO3D_PARAM(P_NODE, Node);      // node ptr
}
//=============================================================================
//=============================================================================
enum ViewMaskType
{
    ViewMask_Normal  = (1 << 0),
    ViewMask_Capture = (1 << 7),
};

const unsigned DEFAULT_IMAGE_SIZE = 512;

//=============================================================================
//=============================================================================
class Lightmap : public Component
{
    URHO3D_OBJECT(Lightmap, Component);
    
public:
    Lightmap(Context* context);
    virtual ~Lightmap();
    
    static void RegisterObject(Context* context);

    void BakeTexture(const String &filepath, unsigned imageSize=DEFAULT_IMAGE_SIZE);

protected:
    void InitRenderSurface(const BoundingBox& worldBoundingBox);
    void HandlePostRender(StringHash eventType, VariantMap& eventData);
    void RestoreStaticModel();
    void SendMsg();
    void Stop();
    void OutputFile();

protected:
    WeakPtr<StaticModel>    staticModel_;
    SharedPtr<Material>     origMaterial_;
    String                  filepath_;

    WeakPtr<Node>           camNode_;
    SharedPtr<Camera>       camera_;
    SharedPtr<Viewport>     viewport_;
    SharedPtr<Texture2D>    renderTexture_;
    WeakPtr<RenderSurface>  renderSurface_;
    SharedPtr<Image>        renderedImage_;

    unsigned                texWidth_;
    unsigned                texHeight_;
    bool                    saveFile_;
};
