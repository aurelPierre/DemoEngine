#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 _view;
    mat4 _proj;
	vec3 _pos;
} cam;

layout(set = 1, binding = 0) uniform ModelData {
    mat4 _model;
} model;

layout(location = 0) in vec3 inPosition;

void main() 
{
    gl_Position = cam._proj * cam._view * model._model * vec4(inPosition, 1.0);
}