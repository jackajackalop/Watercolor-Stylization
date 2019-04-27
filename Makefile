test : \
	dist/test.pgct \
	dist/test.scene
cake: \
	dist/cake.pgct \
	dist/cake.scene
opossum: \
	dist/opossum.pgct \
	dist/opossum.scene
examples:
	./dist/main -blur 0 -save /edge/test0
	./dist/main -blur 10 -save /edge/test1
	./dist/main -blur 20 -save /edge/test2
	./dist/main -density 0.0 -save /granulation/test0
	./dist/main -density 1.0 -save /granulation/test1
	./dist/main -density 2.0 -save /granulation/test2
	./dist/main -dA 0.9 -cangiante 0.1 -dilution 0.1 -save /pigment/test1
	./dist/main -dA 0.1 -cangiante 0.1 -dilution 0.9 -save /pigment/test3
	./dist/main -dA 0.1 -cangiante 0.9 -dilution 0.9 -save /pigment/test5
	./dist/main -dA 0.9 -cangiante 0.1 -dilution 0.9 -save /pigment/test6
	./dist/main -distortion 0 -save /distortion/test0
	./dist/main -distortion 1 -save /distortion/test1
	./dist/main -bleed 0 -save /bleed/test0
	./dist/main -bleed 1 -save /bleed/test1
	./dist/main -show 0 -save 0
	./dist/main -show 1 -save 1
	./dist/main -show 3 -save 2
	./dist/main -blur 10 -show 4 -save 3
	./dist/main -show 5 -save 4
	./dist/main -show 6 -save 5
	./dist/main -show 7 -save 6

dist/test.pgct : meshes/test.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/test.scene : meshes/test.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'
dist/cake.pgct : meshes/cake.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/cake.scene : meshes/cake.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'
dist/opossum.pgct : meshes/hopossum.blend meshes/export-meshes.py
	blender --background --python meshes/export-meshes.py -- '$<' '$@'
dist/opossum.scene : meshes/hopossum.blend meshes/export-scene.py
	blender --background --python meshes/export-scene.py -- '$<' '$@'




