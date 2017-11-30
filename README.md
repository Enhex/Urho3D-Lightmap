# Urho3D Lightmap
  
---
### Description
Lightmap texture baking sample using the GPU.

 
---  
### Information:
Currently, this implementation is written more like a texture baker. It merely generates texture as how the object is lit and shaded in the scene and does not look for the second UV texCoordinates as expected by the Urho3D lightmap shader.  There is no GI implemented, yet.


---  
### Setup:
* only one lightmap texture can be generated at a time, otherwise, the captured view images get dirty
* all static models in the scene must have **ViewMask set to 0x01**
* output files will be placed in Lightmap/BakedTextures
  
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







