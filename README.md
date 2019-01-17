# Game Information
(Note: fill in this portion with information about your game.)

Title: (TODO: your game's title)

Author: (TODO: your name)

Design Document: [TODO: name of design document](TODO: link to design document)

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

TODO: describe the controls and (if needed) goals/strategy.

Changes From The Design Document:

TODO: what did you need to add/remove/modify from the original design? Why?

Good / Bad / Ugly Code:

TODO: provide examples of code you wrote from this project that you think is good (elegant, simple, useful), bad (hack-y, brittle, unreadable), and ugly (particularly inelegant). Provide a sentence or two of justification for the examples.

# Changes In This Base Code

I've changed the main executable name back to 'main' and disabled building the server. However, ```Connection.*pp``` is still around if you decide to do some networking.

I've added a new shader (which you are likely to be modifying) in ```texture_program.*pp```; it supports a shadow-map-based spotlight as well as the existing point and hemisphere lights, and gets its surface color from a texture modulated by a vertex color. This means you can use it with textured objects (with all-white vertex color) and vertex colored objects (with all-white texture).
Scene objects support multiple shader program slots now. This means they can have different uniforms, programs (even geometry) in different rendering passes. The code uses this when rendering the shadow map.

This shader, along with the code in GameMode::draw() which sets it up, is going to be very useful to dig into. Particular shadow map things that were a bit tricky:

 - I'm rendering only the back-facing polygons into the shadow framebuffer; this means there is less Z-fighting on the front faces but can causes a light leak at the back of the pillar which is why there is still a little bit of bias added in the projection matrix.
 - Clip space coordinates are in [-1,1]^3 while depth textures are indexed by values in [0,1]^2 and contain depths in [0,1]. So when transforming the vertex position into shadow map space the coordinates are all scaled by 0.5 and offset by 0.5.
 - The texture_shader does the depth comparison in its shadow lookup by declaring spot_depth_tex as a sampler2DShadow and by setting the TEXTURE_COMPARE_MODE and TEXTURE_COMPARE_FUNC parameters on the texture in GameMode::draw . It also sets filtering mode LINEAR on the texture so that the result in a blend of the four closest depth comparisons.
 - When used on a sampler2DShadow, the textureProj(tex, coord) returns projection and comparison on the supplied texture coordinate -- i.e., (coord.z / coord.w < tex[coord.xy / coord.w] ? 1.0 : 0.0) -- which is very convenient for writing shadow map lookups.

I also added a blur shader that renders the scene to an offscreen framebuffer and then samples and averages it to come up with a sort of lens blur effect. (See the third part of GameMode::draw .)

# Using This Base Code

Before you dive into the code, it helps to understand the overall structure of this repository.
- Files you should read and/or edit:
    - ```main.cpp``` creates the game window and contains the main loop. You should read through this file to understand what it's doing, but you shouldn't need to change things (other than window title, size, and maybe the initial Mode).
    - ```server.cpp``` creates a basic server.
    - ```GameMode.*pp``` declaration+definition for the GameMode, a basic scene-based game mode.
    - ```meshes/export-meshes.py``` exports meshes from a .blend file into a format usable by our game runtime.
    - ```meshes/export-walkmeshes.py``` exports meshes from a given layer of a .blend file into a format usable by the WalkMeshes loading code.
    - ```meshes/export-scene.py``` exports the transform hierarchy of a blender scene to a file.
	- ```Connection.*pp``` networking code.
    - ```Jamfile``` responsible for telling FTJam how to build the project. If you add any additional .cpp files or want to change the name of your runtime executable you will need to modify this.
    - ```.gitignore``` ignores the ```objs/``` directory and the generated executable file. You will need to change it if your executable name changes. (If you find yourself changing it to ignore, e.g., your editor's swap files you should probably, instead be investigating making this change in the global git configuration.)
- Files you should read the header for (and use):
	- ```Sound.*pp``` spatial sound code.
    - ```WalkMesh.*pp``` code to load and walk on walkmeshes.
    - ```MenuMode.hpp``` presents a menu with configurable choices. Can optionally display another mode in the background.
    - ```Scene.hpp``` scene graph implementation, including loading code.
    - ```Mode.hpp``` base class for modes (things that recieve events and draw).
    - ```Load.hpp``` asset loading system. Very useful for OpenGL assets.
    - ```MeshBuffer.hpp``` code to load mesh data in a variety of formats (and create vertex array objects to bind it to program attributes).
    - ```data_path.hpp``` contains a helper function that allows you to specify paths relative to the executable (instead of the current working directory). Very useful when loading assets.
    - ```draw_text.hpp``` draws text (limited to capital letters + *) to the screen.
    - ```compile_program.hpp``` compiles OpenGL shader programs.
    - ```load_save_png.hpp``` load and save PNG images.
- Files you probably don't need to read or edit:
    - ```GL.hpp``` includes OpenGL prototypes without the namespace pollution of (e.g.) SDL's OpenGL header. It makes use of ```glcorearb.h``` and ```gl_shims.*pp``` to make this happen.
    - ```make-gl-shims.py``` does what it says on the tin. Included in case you are curious. You won't need to run it.
    - ```read_chunk.hpp``` contains a function that reads a vector of structures prefixed by a magic number. It's surprising how many simple file formats you can create that only require such a function to access.

## Asset Build Instructions

The ```meshes/export-meshes.py``` script can write mesh data including a variety of attributes (e.g., *p*ositions, *n*ormals, *c*olors, *t*excoords) from a selected layer of a blend file:

```
blender --background --python meshes/export-meshes.py -- meshes/crates.blend:1 dist/crates.pnc
```

The ```meshes/export-scene.py``` script can write the transformation hierarchy of the scene from a selected layer of a blend file, and includes references to meshes (by name):

```
blender --background --python meshes/export-scene.py -- meshes/crates.blend:1 dist/crates.scene
```

The ```meshes/export-walkmeshes.py``` script can writes vertices, normals, and triangle indicies of all meshes on a selected layer of a .blend file:

```
blender --background --python meshes/export-walkmeshes.py -- meshes/crates.blend:3 dist/crates.walkmesh
```

There is a Makefile in the ```meshes``` directory with some example commands of this sort in it as well.

## Runtime Build Instructions

The runtime code has been set up to be built with [FT Jam](https://www.freetype.org/jam/).

### Getting Jam

For more information on Jam, see the [Jam Documentation](https://www.perforce.com/documentation/jam-documentation) page at Perforce, which includes both reference documentation and a getting started guide.

On unixish OSs, Jam is available from your package manager:
```
	brew install ftjam #on OSX
	apt get ftjam #on Debian-ish Linux
```

On Windows, you can get a binary [from sourceforge](https://sourceforge.net/projects/freetype/files/ftjam/2.5.2/ftjam-2.5.2-win32.zip/download),
and put it somewhere in your `%PATH%`.
(Possibly: also set the `JAM_TOOLSET` variable to `VISUALC`.)

### Libraries

This code uses the [libSDL](https://www.libsdl.org/) library to create an OpenGL context, and the [glm](https://glm.g-truc.net) library for OpenGL-friendly matrix/vector types.
On MacOS and Linux, the code should work out-of-the-box if if you have these installed through your package manager.

If you are compiling on Windows or don't want to install these libraries globally there are pre-built library packages available in the
[kit-libs-linux](https://github.com/ixchow/kit-libs-linux),
[kit-libs-osx](https://github.com/ixchow/kit-libs-osx),
and [kit-libs-win](https://github.com/ixchow/kit-libs-win) repositories.
Simply clone into a subfolder and the build should work.

### Building

Open a terminal (or ```x64 Native Tools Command Prompt for VS 2017``` on Windows), change to the directory containing this code, and type:

```
jam
```

That's it. You can use ```jam -jN``` to run ```N``` parallel jobs if you'd like; ```jam -q``` to instruct jam to quit after the first error; ```jam -dx``` to show commands being executed; or ```jam main.o``` to build a specific file (in this case, main.cpp).  ```jam -h``` will print help on additional options.
