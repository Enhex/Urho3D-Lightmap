# Urho3D Lightmap
  
---
### Description
Lightmap texture baking sample using the GPU.  
Currently, this implementation is written more like a texture baker. It merely generates texture as how the object is lit and shadowed in the scene and does not look for the second UV texCoordinates as expected by the Urho3D lightmap shader.  
There is no GI implemented, yet.

#### OpenGL Only
I've applied the same changes to the hlsl shader, and for some reason, I only get black images from the view capture. 

---  
### Setup:
* only one lightmap texture can be generated at a time, otherwise, the captured view images get dirty. For this reason, the **maxThreads_** is set to 1.
* all static models in the scene must have **ViewMask set to 0x01**.
* output files are placed in the **Lightmap/BakedTextures** folder.
  
---
### Screenshots

![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/bakescene.png)
#### Generated Textures
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/bakedtextures.png)

---
### To Build
To build it, unzip/drop the repository into your Urho3D/ folder and build it the same way as you'd build the default Samples that come with Urho3D.
  
---  
### License
The MIT License (MIT)







