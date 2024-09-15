#!/bin/bash

# Set the prefix for shader files
prefix="assets/shaders/"

# Get vertex and fragment shader files
vertex_shader_files=`ls ${prefix}*-vs.glsl`
fragment_shader_files=`ls ${prefix}*-fs.glsl`

# Process vertex shaders
for file in $vertex_shader_files
do
	filename=${file:0:-5}
	glslc -fshader-stage=vertex $file -o ${filename}.spv
done

# Process fragment shaders
for file in $fragment_shader_files
do
	filename=${file:0:-5}
	glslc -fshader-stage=fragment $file -o ${filename}.spv
done
