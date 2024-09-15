@echo off
rem Set the prefix path
set prefix=assets\shaders\

rem Process vertex shaders
for %%f in (%prefix%*-vs.glsl) do (
	glslc -fshader-stage=vertex %%f -o %prefix%%%~nf.spv
)

rem Process fragment shaders
for %%f in (%prefix%*-fs.glsl) do (
	glslc -fshader-stage=fragment %%f -o %prefix%%%~nf.spv
)
