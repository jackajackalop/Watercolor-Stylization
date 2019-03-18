all : \
	dist/test.pgct \
	dist/test.scene \

dist/test.pgct : meshes/spheres.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/test.scene : meshes/spheres.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'


