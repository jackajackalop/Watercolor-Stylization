#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-scene.py -- <infile.blend> <layer> <outfile.scene>

import sys,re

args = []
for i in range(0,len(sys.argv)):
	if sys.argv[i] == '--':
		args = sys.argv[i+1:]

if len(args) != 2:
	print("\n\nUsage:\nblender --background --python export-scene.py -- <infile.blend>[:layer] <outfile.scene>\nExports the transforms of objects in layer (default 1) to a binary blob, indexed by the names of the objects that reference them.\n")
	exit(1)

infile = args[0]
layer = 1
m = re.match(r'^(.*):(\d+)$', infile)
if m:
	infile = m.group(1)
	layer = int(m.group(2))
outfile = args[1]

print("Will export layer " + str(layer) + " from file '" + infile + "' to scene '" + outfile + "'");

import bpy
import mathutils
import struct
import math

#---------------------------------------------------------------------
#Export scene:

bpy.ops.wm.open_mainfile(filepath=infile)

#Scene file format:
# str0 len < char > * [strings chunk]
# xfh0 len < ... > * [transform hierarchy]
# msh0 len < uint uint uint > [hierarchy point + mesh name]
# cam0 len < uint params > [heirarchy point + camera params]
# lig0 len < uint params > [hierarchy point + light params]

strings_data = b""
xfh_data = b""
mesh_data = b""
camera_data = b""
lamp_data = b""

#write_string will add a string to the strings section and return a packed (begin,end) reference:
def write_string(string):
	global strings_data
	begin = len(strings_data)
	strings_data += bytes(string, 'utf8')
	end = len(strings_data)
	return struct.pack('II', begin, end)

obj_to_xfh = dict()

#write_xfh will add an object [and its parents] to the hierarchy section and return a packed (idx) reference:
def write_xfh(obj):
	global xfh_data
	if obj in obj_to_xfh: return obj_to_xfh[obj]
	if obj.parent == None:
		parent_ref = struct.pack('i', -1)
		world_to_parent = mathutils.Matrix()
	else:
		parent_ref = write_xfh(obj.parent)
		world_to_parent = obj.parent.matrix_world.copy()
		world_to_parent.invert()
	ref = struct.pack('i', len(obj_to_xfh))
	obj_to_xfh[obj] = ref
	#print(repr(ref) + ": " + obj.name + " (" + repr(parent_ref) + ")")
	transform = (world_to_parent * obj.matrix_world).decompose()
	#print(repr(transform))

	xfh_data += parent_ref
	xfh_data += write_string(obj.name)
	xfh_data += struct.pack('3f', transform[0].x, transform[0].y, transform[0].z)
	xfh_data += struct.pack('4f', transform[1].x, transform[1].y, transform[1].z, transform[1].w)
	xfh_data += struct.pack('3f', transform[2].x, transform[2].y, transform[2].z)

	return ref

#write_mesh will add an object to the mesh section:
def write_mesh(obj):
	global mesh_data
	assert(obj.type == 'MESH')
	mesh_data += write_xfh(obj) #hierarchy reference
	mesh_data += write_string(obj.data.name) #mesh name
	print("mesh: " + obj.name)

#write_camera will add an object to the camera section:
def write_camera(obj):
	global camera_data
	assert(obj.type == 'CAMERA')
	print("camera: " + obj.name)

	if obj.data.sensor_fit != 'VERTICAL':
		print("  WARNING: camera FOV may seem weird because camera is not in vertical-fit mode.")

	camera_data += write_xfh(obj) #hierarchy reference
	if obj.data.type == 'PERSP':
		camera_data += b"pers"
		fov = math.atan2(0.5*obj.data.sensor_height, obj.data.lens)/math.pi*180.0*2
		print("  Vertical FOV: " + str(fov) + " degrees");
		camera_data += struct.pack('f', fov)
	elif obj.data.type == 'ORTHO':
		camera_data += b"orth"
		print("  Vertical Ortho Size: " + str(obj.data.ortho_scale));
		camera_data += struct.pack('f', obj.data.ortho_scale)
	else:
		assert(False and "Unsupported camera type '" + obj.data.type + "'")
	
	camera_data += struct.pack('ff', obj.data.clip_start, obj.data.clip_end)
		
#write_lamp will add an object to the lamp section:
def write_lamp(obj):
	global lamp_data
	assert(obj.type == 'LAMP')
	print("lamp: " + obj.name)

	lamp_data += write_xfh(obj) #hierarchy reference
	if obj.data.type == 'POINT':
		lamp_data += b"p"
	elif obj.data.type == 'HEMI':
		lamp_data += b"h"
	elif obj.data.type == 'SPOT':
		lamp_data += b"s"
	elif obj.data.type == 'SUN':
		lamp_data += b"d"
	else:
		assert(False and "Unsupported lamp type '" + obj.data.type + "'")
	lamp_data += struct.pack('BBB',
		int(obj.data.color.r * 255),
		int(obj.data.color.g * 255),
		int(obj.data.color.b * 255)
		)
	lamp_data += struct.pack('f', obj.data.energy)
	lamp_data += struct.pack('f', obj.data.distance)
	if obj.data.type == 'SPOT':
		fov = obj.data.spot_size/math.pi*180.0
		print("  Spot size: " + str(fov) + " degrees.")
		lamp_data += struct.pack('f', fov)
	else:
		lamp_data += struct.pack('f', 0.0)
	

for obj in bpy.data.objects:
	if obj.layers[layer-1] == False: continue;
	if obj.type == 'MESH':
		write_mesh(obj)
	elif obj.type == 'CAMERA':
		write_camera(obj)
	elif obj.type == 'LAMP':
		write_lamp(obj)
	else:
		print('Skipping ' + obj.type)

#write the strings chunk and scene chunk to an output blob:
blob = open(outfile, 'wb')
def write_chunk(magic, data):
	blob.write(struct.pack('4s',magic)) #type
	blob.write(struct.pack('I', len(data))) #length
	blob.write(data)

write_chunk(b'str0', strings_data)
write_chunk(b'xfh0', xfh_data)
write_chunk(b'msh0', mesh_data)
write_chunk(b'cam0', camera_data)
write_chunk(b'lmp0', lamp_data)

print("Wrote " + str(blob.tell()) + " bytes to '" + outfile + "'")
blob.close()
