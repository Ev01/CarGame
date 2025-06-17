- [x] Re-add lighting
- [x] Display floor
- [ ] Shadows??
- [x] Orbit Camera
- [x] Multiple physics objects
- [x] Even more physics objects
- [x] Car Body

15/05
- [x] gltf metadata read wheel position
- [x] Don't draw with texture when there is none
- [x] Support for materials defined in model

16/05
- [x] Camera rotation follows car rotation
- [x] More fitting collision box/mesh

17/05
- [x] Backwards driving

18/05
- [x] Controller support
- [x] More snappy camera rotation
- [x] Some audio
- Transmission changes gears

20/05
- [x] Add audio file for handling audio
- [x] Drifting sound

21/05
- Wheel turn lerping

26/05
- Level with mesh shape
- [x] Fix drifting sound stutter (lerp volume?)

28/05
- Fullscreen and window resize
- Point lights and sun light from gltf file

29/05
- [x] Spot lights
- Skybox


- [x] Support variable number of lights
- [x] Refactor car into a struct/class


2/06
- Normal maps

3/06
- PBR Materials (roughness and metallic)

6/06
- [x] Load car parameters at runtime from a file (json?)

7/06
- [x] Prevent camera from going inside objects

9/06
- [x] Fix weird jitter problem
NOTE: Camera Jitter was caused by not updating in Physics Frame. On some frames,
the physics frame doesn't run. This makes the game look laggy and should be
fixed later.

10/06
- [x] Change maps at runtime

11/06
- [x] Properly delete OpenGl textures

12/06
- [x] Two cars
- [x] Split screen

12/06
- Add music

14/06
- [x] FPS / VSync controls on GUI


- [ ] Look into this error. Happened on map1: JPH ASSERT FAILED: C:/Users/Ev/Documents/c/JoltPhysics/Jolt/Geometry/EPAPenetrationDepth.h:115: (!ioV.IsNearZero())
- [ ] Car headlights
- [ ] Edit car paramters with GUI (Dear ImGUI)
- [ ] Fix materials not loading properly when switching maps sometimes
- [ ] Refactor with world struct/class
- [ ] Get model from filename anywhere without loading it twice
- [ ] Time trial with checkpoints
- [ ] Speedometer
- [ ] Luminosity
- [ ] Crash sound effect
- [ ] Adjust drift volume

