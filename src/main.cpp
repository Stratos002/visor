#include <visor.h>

#include <trivex.h>

#include <iostream>
#include <vector>

int main()
{
	Visor::Window window(1000, 700, "test");
	window.open();

	Visor::Camera camera = {};
	camera.fov = 1.2f;
	camera.position.z = -2.0f;

	std::vector<Visor::Entity> entities;

	std::vector<Visor::Mesh::Vertex> vertices;
	std::vector<Visor::ui32> indices;

	{
		TVX_Mesh mesh;
		TVX_loadMeshFromOBJ("../assets/models/cube.obj", &mesh);

		for(uint32_t vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex)
		{
			struct TVX_Position position = mesh.pVertices[vertexIndex].position;

			Visor::Mesh::Vertex vertex;
			vertex.position.x = position.x;
			vertex.position.y = position.y;
			vertex.position.z = position.z;

			vertices.push_back(vertex);
		}

		for(uint32_t vertexI = 0; vertexI < mesh.vertexIndexCount; ++vertexI)
		{
			indices.push_back(mesh.pVertexIndices[vertexI]);
		}

		std::cout << "\n";

		TVX_freeMesh(mesh);
	}

	Visor::Mesh mesh(vertices, indices, "../assets/shaders/intermediate/vertex.spv", "../assets/shaders/intermediate/fragment.spv");

	Visor::Entity entity({ 0.0f, 0.0f, 3.0f }, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, mesh);
	entities.push_back(entity);
	
	Visor::Renderer::start(window, entities);

	while (!window.shouldClose())
	{
		window.pollEvents();
		
		camera.yaw += 0.001f;

		Visor::Renderer::getInstance().render(camera);
	}

	Visor::Renderer::terminate();

	window.close();

	return 0;
}