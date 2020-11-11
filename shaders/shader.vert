#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 _view;
    mat4 _proj;
} cam;

layout(set = 2, binding = 0) uniform ModelData {
    mat4 _model;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;

void main() 
{
	vec4 modelPos = model._model * vec4(inPosition, 1.0);
	fragPos = modelPos.xyz / modelPos.w;
	fragUV = inUV;

    gl_Position = cam._proj * cam._view * modelPos;

	mat3 mNormal = transpose(inverse(mat3(model._model)));
	fragNormal = mNormal * normalize(inNormal);
    fragTangent = mNormal * normalize(inTangent);
}