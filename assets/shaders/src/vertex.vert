#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer 
{
	mat4 viewProjection;
};

layout(set = 1, binding = 0) uniform EntityUniformBuffer 
{
	mat4 transformation;
};

layout(location = 0) in vec3 position;

void main()
{
	gl_Position = viewProjection * transformation * vec4(position, 1.0);
}