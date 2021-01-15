#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 _view;
    mat4 _proj;
	vec3 _pos;
} cam;

layout(location = 0) in vec3 pos;

layout(location = 0) out float near;
layout(location = 1) out float far;
layout(location = 2) out vec3 nearPoint;
layout(location = 3) out vec3 farPoint;
layout(location = 4) out mat4 view;
layout(location = 8) out mat4 proj;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) 
{
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

vec3 gridPlane[4] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, -1, 0)
);

void main() 
{
	vec3 p = gridPlane[gl_VertexIndex].xyz;

	near = 0.1;
	far = 512.0;

	view = cam._view;
	proj = cam._proj;

	nearPoint = UnprojectPoint(p.x, p.y, 0.0, cam._view, cam._proj).xyz; // unprojecting on the near plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0, cam._view, cam._proj).xyz;
    gl_Position = vec4(p, 1.0);
}