test : \
	dist/test.pgct \
	dist/test.scene
cake: \
	dist/cake.pgct \
	dist/cake.scene
opossum: \
	dist/opossum.pgct \
	dist/opossum.scene


dist/test.pgct : meshes/test.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/test.scene : meshes/test.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'
dist/cake.pgct : meshes/cake.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/cake.scene : meshes/cake.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'
dist/opossum.pgct : meshes/opossum.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/opossum.scene : meshes/opossum.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'




