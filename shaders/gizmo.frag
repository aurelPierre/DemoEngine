#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform GizmoData {
	vec3 _color;
} gizmo;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(gizmo._color, 1.0);
}