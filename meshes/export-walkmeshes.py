#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-meshes.py -- <infile.blend>[:layer] <outfile.p[n][c][t]>

import sys,re

args = []
for i in range(0,len(sys.argv)):
	if sys.argv[i] == '--':
		args = sys.argv[i+1:]

if len(args) != 2:
	print("\n\nUsage:\nblender --background --python export-walkmeshes.py -- <infile.blend>[:layer] <outfile.wn>\nExports the meshes referenced by all objects in layer (default 1) to a binary blob, in walkmesh format, indexed by the names of the objects that reference them.\n")
	exit(1)

infile = args[0]
layer = 1
m = re.match(r'^(.*):(\d+)$', infile)
if m:
	infile = m.group(1)
	layer = int(m.group(2))
outfile = args[1]

assert layer >= 1 and layer <= 20

print("Will export meshes referenced from layer " + str(layer) + " of '" + infile + "' to '" + outfile + "'.")

import bpy
import struct

import argparse


bpy.ops.wm.open_mainfile(filepath=infile)

#meshes to write:
to_write = set()
for obj in bpy.data.objects:
	if obj.layers[layer-1] and obj.type == 'MESH':
		to_write.add(obj.data)

#vertex (as vec3), normal (as vec3), and triangle (as uvec3) data from the meshes:
positions = b''
normals = b''
triangles = b''

#strings contains the mesh names:
strings = b''

#index gives offsets into the data (and names) for each mesh:
index = b''

position_count = 0
normal_count = 0
triangle_count = 0

for obj in bpy.data.objects:
	if obj.data in to_write:
		to_write.remove(obj.data)
	else:
		continue

	mesh = obj.data
	name = mesh.name

	print("Writing '" + name + "'...")
	if bpy.context.mode == 'EDIT':
		bpy.ops.object.mode_set(mode='OBJECT') #get out of edit mode (just in case)

	#make sure object is on a visible layer:
	bpy.context.scene.layers = obj.layers
	#select the object and make it the active object:
	bpy.ops.object.select_all(action='DESELECT')
	obj.select = True
	bpy.context.scene.objects.active = obj

	#apply all modifiers (?):
	bpy.ops.object.convert(target='MESH')

	#subdivide object's mesh into triangles:
	bpy.ops.object.mode_set(mode='EDIT')
	bpy.ops.mesh.select_all(action='SELECT')
	bpy.ops.mesh.quads_convert_to_tris(quad_method='BEAUTY', ngon_method='BEAUTY')
	bpy.ops.object.mode_set(mode='OBJECT')

	#compute normals (respecting face smoothing):
	mesh.calc_normals_split()

	#store the beginning indices:
	vertex_begin = position_count
	triangle_begin = triangle_count


	#Helper to write referenced vertices:
	vertex_inds = dict() #for each referenced vertex, store new index
	vertex_normals = [] #for each referenced vertex, store list of normals
	def write_vertex(index, normal):
		global positions, position_count, vertex_refs, vertex_normals
		if index not in vertex_inds:
			vertex_inds[index] = len(vertex_inds)
			vertex_normals.append([])
			positions += struct.pack('fff', *mesh.vertices[index].co)
			position_count += 1
		vertex_normals[vertex_inds[index]].append(normal)
		return struct.pack('I', vertex_begin + vertex_inds[index])

	#write the mesh triangles:
	for poly in mesh.polygons:
		assert(len(poly.loop_indices) == 3)

		#check that faces are CCW-oriented:
		ab =  mesh.vertices[poly.vertices[1]].co - mesh.vertices[poly.vertices[0]].co
		ac =  mesh.vertices[poly.vertices[2]].co - mesh.vertices[poly.vertices[0]].co
		out = ab.cross(ac).normalized()
		d = poly.normal.dot(out)
		assert(d > 0.9)

		for i in range(0,3):
			assert(mesh.loops[poly.loop_indices[i]].vertex_index == poly.vertices[i])
			triangles += write_vertex(poly.vertices[i], mesh.loops[poly.loop_indices[i]].normal)
		triangle_count += 1
	
	#write (and possibly average) the normals:
	for ns in vertex_normals:
		avg = None
		for n in ns:
			if avg == None: avg = n
			else: avg = avg + n
		avg = avg / len(ns)
		for n in ns:
			diff = (n - avg).length
			if diff > 0.001:
				print("Normal " + str(n) + " different than average " + str(avg) + " of " + str(len(ns)) + ".")
		normals += struct.pack('fff', *avg)
		normal_count += 1
	
	assert(normal_count == position_count)

	vertex_end = position_count
	triangle_end = triangle_count

	assert(vertex_end - vertex_begin == len(vertex_inds))

	#record mesh name, vertex range, and triangle range:
	name_begin = len(strings)
	strings += bytes(name, "utf8")
	name_end = len(strings)
	index += struct.pack('II', name_begin, name_end)
	index += struct.pack('II', vertex_begin, vertex_end)
	index += struct.pack('II', triangle_begin, triangle_end)


#check that we wrote as much data as anticipated:
assert(position_count * 3*4 == len(positions))
assert(normal_count * 3*4 == len(normals))

#write the data chunk and index chunk to an output blob:
blob = open(outfile, 'wb')

def write_chunk(magic, data):
	blob.write(struct.pack('4s',magic)) #type
	blob.write(struct.pack('I', len(data))) #length
	blob.write(data)

#first chunk: the positions
write_chunk(b'p...', positions)
write_chunk(b'n...', normals)
write_chunk(b'tri0', triangles)
write_chunk(b'str0', strings)
write_chunk(b'idxA', index)
wrote = blob.tell()
blob.close()

print("Wrote " + str(wrote) + " bytes [== " +
	str(len(positions)+8) + " bytes of positions + " +
	str(len(normals)+8) + " bytes of normals + " +
	str(len(triangles)+8) + " bytes of triangles + " +
	str(len(strings)+8) + " bytes of strings + " +
	str(len(index)+8) + " bytes of index] to '" + outfile + "'")
