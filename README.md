# Urho3D Lightmap
  
---
### Description
Lightmap texture baking sample.  
Generates textures on texCoord2:
* direct lighting
* indirect lighting

#### OpenGL Only
I've applied the same changes to the hlsl shader, and for some reason, I only get black images from the view capture. I will not be pursuing this fix but it's fully implementation using OpenGL.

---  
### Setup:
* only one direct lightmap texture can be generated at a time, otherwise, the captured view images result in inaccuracies.
* output files are placed in the **Lightmap/BakedTextures** folder.
  
---
### Screenshots
#### Direct Lighting Only
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/directonly1.png)  
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/directonly2.png)  

#### Indirect Lighting
Shown shaded with DiffLightMap and NoTextureLightMap techniques.
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/indirect1.png)  
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/indirect2.png)  
  
#### Direct Lighting Baked Textures
![alt tag](https://github.com/Lumak/Urho3D-Lightmap/blob/master/screenshot/bakedtextures.png)  

---
### To Build
To build it, unzip/drop the repository into your Urho3D/ folder and build it the same way as you'd build the default Samples that come with Urho3D.
  
---  
### License
The MIT License (MIT)







