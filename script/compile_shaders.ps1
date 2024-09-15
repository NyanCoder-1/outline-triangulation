# Set the prefix path
$prefix = "assets/shaders/"

# Get the vertex and fragment shader files
$vertexShaderFiles = Get-ChildItem "$prefix"*-vs.glsl
$fragmentShaderFiles = Get-ChildItem "$prefix"*-fs.glsl

# Process vertex shaders
foreach ($file in $vertexShaderFiles) {
	$filename = $file.BaseName
	glslc -fshader-stage=vertex "$prefix$file" -o "$prefix$filename.spv"
}

# Process fragment shaders
foreach ($file in $fragmentShaderFiles) {
	$filename = $file.BaseName
	glslc -fshader-stage=fragment "$prefix$file" -o "$prefix$filename.spv"
}