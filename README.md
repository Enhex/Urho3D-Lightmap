# Urho3D Lightmap
  
---
### Description
Lightmap generator for Urho3D. This implementation is based on **Hugo Elias's Radiosity**. Except, I haven't implemented the skipping of pixels, evaluate, and interpolate part, but use brute force pixel processing instead. And I also use hemisphere instead of hemicube, which is used by all implementations that I've seen that use Hugo's method.

Using Urho3D's tech., indirect lighting process is **blazing fast**.  And the accuracy of the lightmap only depends on the resolution that you choose.  

Here's an example of the lightmap image created at 512x512 resolution:
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/node8_lightmap.png)  

#### Generates textures on texCoord2:
* direct baked lighting
* generated lightmaps
* indirect baked lighting

**Note:** baked textures are generated via GPU.

#### Full OpenGL Implementation
I've applied the same changes to the hlsl shader for texture baking, but for some reason, I only get black images from the view capture. I will not be pursuing this fix, however, you still can generate lightmap textures. Essentially, that's all you need to achieve direct and indirect lighting shown in the below screenshots.

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
*Shown shaded with DiffLightMap and NoTextureLightMap techniques.*
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







