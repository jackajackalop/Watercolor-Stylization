all : \
	dist/test.pgct \
	dist/test.scene \

dist/test.pgct : meshes/test.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/test.scene : meshes/test.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'


