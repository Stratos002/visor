#version 450

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 outColor;

vec3 lightDir = normalize(vec3(0.1, -1.0, 0.5));
vec3 color = vec3(1.0, 1.0, 0.4);

void main()
{
	outColor = vec4(color * max(0.0, dot(normal, lightDir)), 1.0);
}