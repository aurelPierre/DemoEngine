#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 _view;
    mat4 _proj;
	vec3 _pos;
} cam;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragUVW;

void main() 
{
	fragUVW = inPosition;
	fragUVW.xy *= -1.0;

    gl_Position = cam._proj * cam._view * vec4(inPosition.xyz, 1);
}