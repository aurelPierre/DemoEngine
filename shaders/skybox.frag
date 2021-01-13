#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform samplerCube skyboxCubeMap;

layout (location = 0) in vec3 fragUVW;

layout (location = 0) out vec4 outColor;

void main() 
{
	outColor = texture(skyboxCubeMap, fragUVW);
}