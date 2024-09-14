#!/bin/bash

prefix="assets/shaders/"

vertex_shader_files=`ls ${prefix}*-vs.glsl`
fragment_shader_files=`ls ${prefix}*-fs.glsl`

for file in $vertex_shader_files
do
	filename=${file:0:-5}
	glslc -fshader-stage=vertex $file -o ${filename}.spv
done

for file in $fragment_shader_files
do
	filename=${file:0:-5}
	glslc -fshader-stage=fragment $file -o ${filename}.spv
done
