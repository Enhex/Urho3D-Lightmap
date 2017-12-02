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
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/View.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderSurface.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <cstdio>

#include "Lightmap.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
Lightmap::Lightmap(Context* context)
    : Component(context)
    , origViewMask_(0)
    , texWidth_(512)
    , texHeight_(512)
    , saveFile_(true)
{
}

Lightmap::~Lightmap()
{
}

void Lightmap::RegisterObject(Context* context)
{
    context->RegisterFactory<Lightmap>();
}

bool Lightmap::InitModelSetting(unsigned tempViewMask)
{
    bool success = false;

    if (node_)
    {
        staticModel_ = node_->GetComponent<StaticModel>();

        if (staticModel_)
        {
            origMaterial_ = staticModel_->GetMaterial()->Clone();
            origViewMask_ = staticModel_->GetViewMask();

            // assign temp view mask during the process
            tempViewMask_ = tempViewMask;
            staticModel_->SetViewMask(tempViewMask_);

            success = true;
        }
    }

    return success;
}

bool Lightmap::RestoreModelSetting()
{
    bool success = false;

    if (staticModel_)
    {
        staticModel_->SetMaterial(origMaterial_);
        staticModel_->SetViewMask(origViewMask_);

        success = true;
    }

    return success;
}

void Lightmap::BakeDirectLight(const String &filepath, unsigned imageSize)
{
    // InitModelSetting() fn must be called 1st
    if (staticModel_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        texWidth_ = texHeight_ = imageSize;
        filepath_ = filepath;

        // clone mat to make changes
        SharedPtr<Material> dupMat = staticModel_->GetMaterial()->Clone();
        staticModel_->SetMaterial(dupMat);

        // choose appropriate bake technique
        Technique *technique = dupMat->GetTechnique(0);
        if (technique->GetName().Find("NoTexture") != String::NPOS)
        {
            dupMat->SetTechnique(0, cache->GetResource<Technique>("Lightmap/Techniques/NoTextureBake.xml"));
        }
        else
        {
            dupMat->SetTechnique(0, cache->GetResource<Technique>("Lightmap/Techniques/DiffBake.xml"));
        }

        //**NOTE** change mask
        staticModel_->SetViewMask(staticModel_->GetViewMask() | ViewMask_Capture);

        // setup for direct lighting
        InitDirectLightSettings(staticModel_->GetWorldBoundingBox());

        SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Lightmap, HandlePostRenderDirectLighting));
    }
}

void Lightmap::BeginIndirectLighting(const String &filepath, unsigned imageSize)
{
    // InitModelSetting() fn must be called 1st
    if (staticModel_)
    {
        texWidth_ = texHeight_ = imageSize;
        filepath_ = filepath;

        // hemisphere solid angle
        solidangle_ = 2.0f * M_PI /(float)(imageSize * imageSize);

        // state change
        SetState(State_CreateGeomData);
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Lightmap, HandleUpdate));
    }
}

void Lightmap::SwatToLightmapTechnique()
{
    // InitModelSetting() fn must be called 1st
    if (staticModel_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        // clone mat to make changes
        SharedPtr<Material> dupMat = staticModel_->GetMaterial()->Clone();
        staticModel_->SetMaterial(dupMat);

        // choose appropriate bake technique
        Technique *technique = dupMat->GetTechnique(0);
        if (technique->GetName().Find("NoTexture") != String::NPOS)
        {
            dupMat->SetTechnique(0, cache->GetResource<Technique>("Lightmap/Techniques/NoTextureLightMap.xml"));
        }
        else
        {
            dupMat->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffLightMap.xml"));
        }

        dupMat->SetShaderParameter("MatEmissiveColor", Color::BLACK);
        SharedPtr<Texture2D> emissiveTex(new Texture2D(context_));
        emissiveTex->SetData(indirectLightImage_);
        dupMat->SetTexture(TU_EMISSIVE, emissiveTex);
    }
}

void Lightmap::InitDirectLightSettings(const BoundingBox& worldBoundingBox)
{
    camNode_ = GetScene()->CreateChild("RenderCamera");

    // set campos right at the model
    Vector3 halfSize = worldBoundingBox.HalfSize();
    camNode_->SetWorldPosition(worldBoundingBox.Center() - Vector3(0, 0, halfSize.z_ ));

    camera_ = camNode_->CreateComponent<Camera>();
    camera_->SetFov(90.0f);
    camera_->SetNearClip(0.0001f);
    camera_->SetAspectRatio(1.0f);
    camera_->SetOrthographic(true);
    camera_->SetOrthoSize(Vector2((float)texWidth_, (float)texHeight_));

    //**NOTE** change mask
    camera_->SetViewMask(ViewMask_Capture);

    viewport_ = new Viewport(context_, GetScene(), camera_);
    viewport_->SetRenderPath(GetSubsystem<Renderer>()->GetViewport(0)->GetRenderPath());

    // Construct render surface 
    renderTexture_ = new Texture2D(context_);
    renderTexture_->SetNumLevels(1);
    renderTexture_->SetSize(texWidth_, texHeight_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    renderTexture_->SetFilterMode(FILTER_BILINEAR);
    
    renderSurface_ = renderTexture_->GetRenderSurface();
    renderSurface_->SetViewport(0, viewport_);
    renderSurface_->SetUpdateMode(SURFACE_UPDATEALWAYS);
}

void Lightmap::InitIndirectLightSettings()
{
    camNode_ = GetScene()->CreateChild("RenderCamera");

    camera_ = camNode_->CreateComponent<Camera>();
    // using hemisphere
    camera_->SetFov(180.0f);
    camera_->SetNearClip(0.0001f);
    camera_->SetFarClip(300.0f);
    camera_->SetAspectRatio(1.0f);

    viewport_ = new Viewport(context_, GetScene(), camera_);
    viewport_->SetRenderPath(GetSubsystem<Renderer>()->GetViewport(0)->GetRenderPath());

    // Construct render surface 
    renderTexture_ = new Texture2D(context_);
    renderTexture_->SetNumLevels(1);
    renderTexture_->SetSize(texWidth_, texHeight_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    renderTexture_->SetFilterMode(FILTER_BILINEAR);
    
    renderSurface_ = renderTexture_->GetRenderSurface();
    renderSurface_->SetViewport(0, viewport_);
    renderSurface_->SetUpdateMode(SURFACE_UPDATEALWAYS);
}

unsigned Lightmap::GetState()
{
    MutexLock lock(mutexStateLock_);
    return stateProcess_;
}

void Lightmap::SetState(unsigned state)
{
    MutexLock lock(mutexStateLock_);
    stateProcess_ = state;
}

void Lightmap::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    ForegroundProcess();
}

void Lightmap::ForegroundProcess()
{
    switch (GetState())
    {
    case State_CreateGeomData:
        {
            SetupGeomData();
            SetState(State_CreatePixelData);

            // start background process
            threadProcess_ = new HelperThread<Lightmap>(this, &Lightmap::BackgroundBuildPixelData);
            threadProcess_->Start();
        }
        break;

    case State_CreatePixelData:
        // wait for background
        break;

    case State_IndirectLightBegin:
        {
            // kill thread
            threadProcess_ = NULL;

            // setup for indirect lighting
            InitIndirectLightSettings();

            // and set 1st cam setting
            curPixelIdx_ = 0;
            SetCameraPosRotForCapture();
            timerIndirect_.Reset();

            SendTriangleInfoMsg();

            SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Lightmap, HandlePostRenderIndirectLighting));

            SetState(State_IndirectLightProcess);
        }
        break;
    }
}

void Lightmap::BackgroundBuildPixelData(void *data)
{
    Lightmap *lightmap = (Lightmap*)data;

    if (lightmap->GetState() == State_CreatePixelData)
    {
        lightmap->SetupPixelData();
        lightmap->SetState(State_IndirectLightBegin);
    }
}

void Lightmap::SendTriangleInfoMsg()
{
    using namespace TriangleInfo;

    unsigned prevIdx = M_MAX_UNSIGNED;
    unsigned triCnt = 0;
    for ( unsigned i = 0; i < pixelData_.Size(); ++i )
    {
        if (prevIdx != pixelData_[i].triIdx_)
        {
            prevIdx = pixelData_[i].triIdx_;
            ++triCnt;
        }
    }

    VariantMap& eventData  = GetEventDataMap();
    eventData[P_TRICNT]    = triCnt;

    SendEvent(E_TRIANGLEINFO, eventData);
}

void Lightmap::SendTriangleCompleteMsg()
{
    using namespace TriangleCompleted;

    VariantMap& eventData  = GetEventDataMap();

    SendEvent(E_TRIANGLECOMPLETED, eventData);
}

void Lightmap::SendIndirectCompleteMsg()
{
    using namespace IndirectCompleted;

    VariantMap& eventData  = GetEventDataMap();
    eventData[P_NODE]      = node_;

    SendEvent(E_INDIRECTCOMPLETED, eventData);
}

void Lightmap::SetCameraPosRotForCapture()
{
    PixelPoint &pixelPoint = pixelData_[curPixelIdx_];

    camNode_->SetPosition(pixelPoint.pos_);
    camNode_->SetDirection(pixelPoint.normal_);
}

void Lightmap::RestoreTempViewMask()
{
    // restore model's orig state
    staticModel_->SetMaterial(origMaterial_);
    staticModel_->SetViewMask(tempViewMask_);
}

void Lightmap::Stop()
{
    // remove
    camNode_->Remove();
    camNode_ = NULL;
    viewport_ = NULL;
    renderSurface_ = NULL;
    renderTexture_ = NULL;

    UnsubscribeFromEvent(E_ENDFRAME);
}

void Lightmap::OutputFile()
{
    // generate output file
    if (saveFile_)
    {
        String name = ToString("node%u_direct.png", node_->GetID());
        String path = filepath_ + name;

        directLightImage_->SavePNG(path);
    }
}

void Lightmap::SendDirectLightMsg()
{
    using namespace DirectLightmapDone;

    VariantMap& eventData  = GetEventDataMap();
    eventData[P_NODE]      = node_;

    SendEvent(E_DIRECTLIGHTINGDONE, eventData);
}

void Lightmap::HandlePostRenderDirectLighting(StringHash eventType, VariantMap& eventData)
{
    // get image prior to deleting the surface
    directLightImage_ = renderTexture_->GetImage();

    RestoreTempViewMask();

    Stop();

    OutputFile();

    SendDirectLightMsg();
}

void Lightmap::HandlePostRenderIndirectLighting(StringHash eventType, VariantMap& eventData)
{
    // get image
    indirectLightImage_ = renderTexture_->GetImage();

    // average color
    pixelData_[curPixelIdx_].col_ = Color(0,0,0,0);
    for ( int y = 0; y < indirectLightImage_->GetHeight(); ++y )
    {
        for ( int x = 0; x < indirectLightImage_->GetWidth(); ++x )
        {
            pixelData_[curPixelIdx_].col_ += indirectLightImage_->GetPixel(x, y);
        }
    }
    pixelData_[curPixelIdx_].col_ = pixelData_[curPixelIdx_].col_ * solidangle_;

    // check tri complete
    unsigned prevTriIdx = pixelData_[curPixelIdx_].triIdx_;

    if (++curPixelIdx_ >= pixelData_.Size())
    {
        renderSurface_ = NULL;
        UnsubscribeFromEvent(E_ENDFRAME);

        float elapsed = (float)((long)timerIndirect_.GetMSec(false))/1000.0f;
        char buff[20];
        sprintf(buff, "%.2f sec.", elapsed);
        URHO3D_LOGINFO(ToString("node%u: indirect completion = ", node_->GetID()) + String(buff));

        // write final image
        indirectLightImage_->Clear(Color(0,0,0,0));
        for ( unsigned i = 0; i < pixelData_.Size(); ++i )
        {
            int x = (int)(pixelData_[i].uv_.x_ * (float)indirectLightImage_->GetWidth());
            int y = (int)(pixelData_[i].uv_.y_ * (float)indirectLightImage_->GetHeight());
            indirectLightImage_->SetPixel(x, y, pixelData_[i].col_);
        }

        String name = ToString("node%u_indirect.png", node_->GetID());
        String path = filepath_ + name;

        indirectLightImage_->SavePNG(path);

        // send msgs
        SendTriangleCompleteMsg();
        SendIndirectCompleteMsg();
    }
    else
    {
        SetCameraPosRotForCapture();

        // send tri complete msg
        unsigned nextTriIdx = pixelData_[curPixelIdx_].triIdx_;

        if (nextTriIdx != prevTriIdx)
        {
            SendTriangleCompleteMsg();
        }
    }
}

void Lightmap::SetupGeomData()
{
    Model *model = staticModel_->GetModel();
    Geometry *geometry = model->GetGeometry(0, 0);
    VertexBuffer *vbuffer = geometry->GetVertexBuffer(0);
    const unsigned char *vertexData = (const unsigned char*)vbuffer->Lock(0, vbuffer->GetVertexCount());

    // transform in world space
    const Matrix3x4 objMatrix = node_->GetTransform();
    const Quaternion objRotation = node_->GetWorldRotation();

    // populate geom data
    if (vertexData)
    {
        unsigned elementMask = vbuffer->GetElementMask();
        unsigned numVertices = vbuffer->GetVertexCount();
        unsigned vertexSize = vbuffer->GetVertexSize();

        geomData_.Resize(numVertices);

        for ( unsigned i = 0; i < numVertices; ++i )
        {
            unsigned char *dataAlign = (unsigned char *)(vertexData + i * vertexSize);
            GeomData &geom = geomData_[i];

            if (elementMask & MASK_POSITION)
            {
                geom.pos_ = *reinterpret_cast<const Vector3*>(dataAlign);
                geom.pos_ = objMatrix * geom.pos_;
                dataAlign += sizeof(Vector3);
            }
            if (elementMask & MASK_NORMAL)
            {
                geom.normal_ = *reinterpret_cast<const Vector3*>(dataAlign);
                geom.normal_ = objRotation * geom.normal_;
                dataAlign += sizeof(Vector3);
            }
            if (elementMask & MASK_COLOR)
            {
                dataAlign += sizeof(unsigned);
            }
            if (elementMask & MASK_TEXCOORD1)
            {
                dataAlign += sizeof(Vector2);
            }
            if (elementMask & MASK_TEXCOORD2)
            {
                geom.uv_ = *reinterpret_cast<const Vector2*>(dataAlign);
            }
        }
        vbuffer->Unlock();
    }

    // get indices
    IndexBuffer *ibuffer = geometry->GetIndexBuffer();
    void *pdata = ibuffer->Lock(0, ibuffer->GetIndexCount());

    if (pdata)
    {
        numIndices_ = ibuffer->GetIndexCount();
        indexSize_ = ibuffer->GetIndexSize();

        if (indexSize_ == sizeof(unsigned short))
        {
            indexBuffShort_ = new unsigned short[numIndices_];
            memcpy(indexBuffShort_.Get(), pdata, numIndices_ * indexSize_);
        }
        else
        {
            indexBuff_ = new unsigned[numIndices_];
            memcpy(indexBuff_.Get(), pdata, numIndices_ * indexSize_);
        }

        ibuffer->Unlock();
    }
}

void Lightmap::SetupPixelData()
{
    const int texSizeX = texWidth_;
    const int texSizeY = texHeight_;
    const float texSizeXINV = 1.0f/(float)texSizeX;
    const float texSizeYINV = 1.0f/(float)texSizeY;

    pixelData_.Reserve((int)((float)(texSizeX * texSizeY) * 1.05f));

    // build sh coeff
    for( unsigned i = 0; i < numIndices_; i += 3 )
    {
        const unsigned idx0 = (const unsigned)((indexSize_ == sizeof(unsigned short))?(indexBuffShort_[i+0]):(indexBuff_[i+0]));
        const unsigned idx1 = (const unsigned)((indexSize_ == sizeof(unsigned short))?(indexBuffShort_[i+1]):(indexBuff_[i+1]));
        const unsigned idx2 = (const unsigned)((indexSize_ == sizeof(unsigned short))?(indexBuffShort_[i+2]):(indexBuff_[i+2]));

        const Vector3 &v0 = geomData_[idx0].pos_;	
        const Vector3 &v1 = geomData_[idx1].pos_;	
        const Vector3 &v2 = geomData_[idx2].pos_;	

        const Vector3 &n0 = geomData_[idx0].normal_;   
        const Vector3 &n1 = geomData_[idx1].normal_;   
        const Vector3 &n2 = geomData_[idx2].normal_;  

        const Vector2 &uv0 = geomData_[idx0].uv_;
        const Vector2 &uv1 = geomData_[idx1].uv_;
        const Vector2 &uv2 = geomData_[idx2].uv_;

        float xMin = 1.0f;	
        float xMax = 0.0f;	
        float yMin = 1.0f;
        float yMax = 0.0f;

        if (uv0.x_ < xMin) xMin = uv0.x_; 
        if (uv1.x_ < xMin) xMin = uv1.x_; 
        if (uv2.x_ < xMin) xMin = uv2.x_; 

        if (uv0.x_ > xMax) xMax = uv0.x_; 
        if (uv1.x_ > xMax) xMax = uv1.x_; 
        if (uv2.x_ > xMax) xMax = uv2.x_; 

        if (uv0.y_ < yMin) yMin = uv0.y_; 
        if (uv1.y_ < yMin) yMin = uv1.y_; 
        if (uv2.y_ < yMin) yMin = uv2.y_; 

        if (uv0.y_ > yMax) yMax = uv0.y_;
        if (uv1.y_ > yMax) yMax = uv1.y_;
        if (uv2.y_ > yMax) yMax = uv2.y_;

        const int pixMinX = (int)Max((float)floor(xMin*texSizeX)-1, 0.0f); 
        const int pixMaxX = (int)Min((float)ceil(xMax*texSizeX)+1, (float)texSizeX); 
        const int pixMinY = (int)Max((float)floor(yMin*texSizeY)-1, 0.0f); 
        const int pixMaxY = (int)Min((float)ceil(yMax*texSizeY)+1, (float)texSizeY);

        Vector3 point, normal, bary;
        Vector2 uv;

        for ( int x = pixMinX; x < pixMaxX; ++x ) 
        {
            for ( int y = pixMinY; y < pixMaxY; ++y ) 
            {
                uv = Vector2((float)x * texSizeXINV, (float)y * texSizeYINV);
                bary = Barycentric(uv0, uv1, uv2, uv);

                if (BaryInsideTriangle(bary))
                {
                    point = bary.x_ * v0 + bary.y_ * v1 + bary.z_ * v2;
                    normal = (bary.x_ * n0 + bary.y_ * n1 + bary.z_ * n2).Normalized();

                    // save pixel pt data
                    pixelData_.Resize(pixelData_.Size() + 1);
                    PixelPoint &pixelPt = pixelData_[pixelData_.Size() - 1];

                    pixelPt.triIdx_ = i;
                    pixelPt.pos_    = point;
                    pixelPt.normal_ = normal;
                    pixelPt.uv_     = uv;
                }
            }
        }
    }
}





