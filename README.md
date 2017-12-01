# Urho3D Lightmap
  
---
### Description
Lightmap texture baking sample.  
Currently, this implementation is written more like a texture baker. It generates direct lighting textures and does not look for the second UV texCoordinates as expected by the Urho3D lightmap shader.  
There is no GI implemented as yet.

#### OpenGL Only
I've applied the same changes to the hlsl shader, and for some reason, I only get black images from the view capture. I will not be pursuing this fix but will continue with the full implementation using OpenGL.

---  
### Setup:
* only one direct lightmap texture can be generated at a time, otherwise, the captured view images result in inaccuracies.
* all static models in the scene must have **ViewMask set to 0x01** (soon to be absolete).
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







