# Urho3D Lightmap
  
---
### Description
Lightmap texture baking sample.  

Generates textures on texCoord2:
* direct baked lighting
* generated lightmaps
* indirect baked lighting

**Note:** baked textured are generated via GPU.

#### OpenGL Only
I've applied the same changes to the hlsl shader, and for some reason, I only get black images from the view capture. I will not be pursuing this fix but it's fully implementated using OpenGL.

---  
### Setup:
* only one baked texture can be generated at a time, otherwise, the captured view images result in inaccuracies.
* once the lightmapping process is complete, what's shown in the scene are shaded with xxLightmap technique.
* output files are placed in the **Lightmap/BakedTextures** folder.
  
---
### Screenshots
#### Direct Lighting Only
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/directonly1.png)  
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/directonly2.png)  

#### Direct and Indirect Lighting
**Shown shaded with DiffLightMap and NoTextureLightMap techniques.**
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/indirect1.png)  
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/indirect2.png)  
  
#### Direct Lighting Baked Textures
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/bakedtextures.png)  

#### Generated Lightmaps
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/generatedLightmaps.png)  

#### Indirect Lighting Baked Textures
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/bakedIndirectTextures.png)  

---
### To Build
To build it, unzip/drop the repository into your Urho3D/ folder and build it the same way as you'd build the default Samples that come with Urho3D.
  
---  
### License
The MIT License (MIT)







