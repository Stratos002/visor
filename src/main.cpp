#include <visor.h>

#include <trivex.h>

#include <string>
#include <iostream>
#include <vector>
#include <random>

static Visor::Mesh loadMesh(const std::string& meshPath)
{
	std::vector<Visor::Mesh::Vertex> vertices;
	std::vector<Visor::ui32> indices;

	TVX_Mesh TVXMesh;
	TVX_loadMeshFromOBJ(meshPath.c_str(), &TVXMesh);

	for(uint32_t vertexIndex = 0; vertexIndex < TVXMesh.vertexCount; ++vertexIndex)
	{
		struct TVX_Position position = TVXMesh.pVertices[vertexIndex].position;
		struct TVX_Normal normal = TVXMesh.pVertices[vertexIndex].normal;

		Visor::Mesh::Vertex vertex = {};
		vertex.position.x = position.x;
		vertex.position.y = position.y;
		vertex.position.z = position.z;
		vertex.normal.x = normal.x;
		vertex.normal.y = normal.y;
		vertex.normal.z = normal.z;

		vertices.push_back(vertex);
	}

	for(uint32_t vertexIndexIndex = 0; vertexIndexIndex < TVXMesh.vertexIndexCount; ++vertexIndexIndex)
	{
		indices.push_back(TVXMesh.pVertexIndices[vertexIndexIndex]);
	}

	TVX_destroyMesh(TVXMesh);

	return Visor::Mesh(vertices, indices, "../assets/shaders/intermediate/vertex.spv", "../assets/shaders/intermediate/fragment.spv");
}

static void createEntities(std::vector<Visor::Entity>& entities)
{
	Visor::Mesh mesh = loadMesh("../assets/models/teapot.obj");

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(-5, 5);

	const Visor::ui32 entityCount = 10;
	const Visor::f32 scale = 0.1f;

	for(Visor::ui32 entityIndex = 0; entityIndex < entityCount; ++entityIndex)
	{
		Visor::Entity entity({(Visor::f32)distrib(gen), (Visor::f32)distrib(gen), (Visor::f32)distrib(gen)}, scale, scale, scale, 0.0f, 0.0f, 0.0f, mesh);	
		entities.push_back(entity);
	}
}

static void updateCamera(Visor::Camera& camera)
{
	Visor::f32 speed = 0.0f;
	if(Visor::InputSystem::getInstance().isKeyPressed(Visor::InputSystem::Key::W))
	{
		speed += 0.01f;
	}
	if(Visor::InputSystem::getInstance().isKeyPressed(Visor::InputSystem::Key::S))
	{
		speed -= 0.01f;
	}

	Visor::f32 currentMouseX = 0.0f;
	Visor::f32 currentMouseY = 0.0f;
	Visor::InputSystem::getInstance().getMousePosition(currentMouseX, currentMouseY);

	Visor::f32 previousMouseX = 0.0f;
	Visor::f32 previousMouseY = 0.0f;
	Visor::InputSystem::getInstance().getPreviousMousePosition(previousMouseX, previousMouseY);

	Visor::f32 dx = previousMouseX - currentMouseX;
	Visor::f32 dy = previousMouseY - currentMouseY;

	const Visor::f32 yaw = dx * 0.01f;
	const Visor::f32 pitch = dy * 0.01f;

	camera.yaw += yaw;
	camera.pitch += pitch;
	
	Visor::Vector3<Visor::f32> forward = {
			0.0f,
			0.0f,
			1.0f,
	};
	const Visor::Matrix3<Visor::f32> rotation = (Visor::Matrix4<Visor::f32>::getRotation(camera.yaw, camera.pitch, 0.0f)).getUpperLeft();
	const Visor::Vector3<Visor::f32> rotatedForward = rotation * forward;
	const Visor::Vector3<Visor::f32> scaledRotatedForwardVector = rotatedForward * speed;
	
	camera.position = camera.position + scaledRotatedForwardVector;
}

static void updateEntities(std::vector<Visor::Entity>& entities)
{
	for(Visor::ui32 entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
	{
		Visor::Entity& entity = entities[entityIndex];
		entity.yaw += 0.001;
	}
}

int main()
{
	std::vector<Visor::Entity> entities;
	createEntities(entities);

	Visor::InputSystem::start();
	Visor::WindowSystem::start(1000, 700);
	Visor::RenderSystem::start();

	Visor::Camera camera = {};
	camera.fov = 1.2f;
	camera.position.z = -2.0f;
	
	while(!Visor::WindowSystem::getInstance().getWindow().shouldClose())
	{
		Visor::InputSystem::getInstance().update();
		Visor::WindowSystem::getInstance().pollEvents();

		updateCamera(camera);
		updateEntities(entities);
		
		Visor::RenderSystem::getInstance().render(camera, entities);
	}

	Visor::RenderSystem::terminate();
	Visor::WindowSystem::terminate();
	Visor::InputSystem::terminate();

	return 0;
}