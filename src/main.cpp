#include <visor.h>

#include <trivex.h>

#include <iostream>
#include <vector>

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

	Visor::f32 dx = currentMouseX - previousMouseX;
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

int main()
{
	std::vector<Visor::Entity> entities;
	std::vector<Visor::Mesh::Vertex> vertices;
	std::vector<Visor::ui32> indices;

	{
		TVX_Mesh TVXMesh;
		TVX_loadMeshFromOBJ("../assets/models/cube.obj", &TVXMesh);

		for(uint32_t vertexIndex = 0; vertexIndex < TVXMesh.vertexCount; ++vertexIndex)
		{
			struct TVX_Position position = TVXMesh.pVertices[vertexIndex].position;

			Visor::Mesh::Vertex vertex;
			vertex.position.x = position.x;
			vertex.position.y = position.y;
			vertex.position.z = position.z;

			vertices.push_back(vertex);
		}

		for(uint32_t vertexI = 0; vertexI < TVXMesh.vertexIndexCount; ++vertexI)
		{
			indices.push_back(TVXMesh.pVertexIndices[vertexI]);
		}

		std::cout << "\n";

		TVX_destroyMesh(TVXMesh);
		
		Visor::Mesh mesh(vertices, indices, "../assets/shaders/intermediate/vertex.spv", "../assets/shaders/intermediate/fragment.spv");
		Visor::Entity entity({ 0.0f, 0.0f, 3.0f }, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, mesh);
		entities.push_back(entity);
	}

	Visor::InputSystem::start();
	Visor::WindowSystem::start(1000, 700);
	Visor::RenderSystem::start(entities);

	Visor::Camera camera = {};
	camera.fov = 1.2f;
	camera.position.z = -2.0f;
	
	while(!Visor::WindowSystem::getInstance().getWindow().shouldClose())
	{
		Visor::InputSystem::getInstance().update();
		Visor::WindowSystem::getInstance().pollEvents();

		updateCamera(camera);
		
		Visor::RenderSystem::getInstance().render(camera);
	}

	Visor::RenderSystem::terminate();
	Visor::WindowSystem::terminate();
	Visor::InputSystem::terminate();

	return 0;
}