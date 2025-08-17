#include <visor.h>

#include <iostream>
#include <vector>

int main()
{
	Visor::Window window(1500, 1200, "test");
	window.open();

	Visor::Camera camera = {};
	camera.fov = 1.2f;

	std::vector<Visor::Entity> entities;

	std::vector<Visor::Mesh::Vertex> vertices = {
		{-0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}
	};

	std::vector<Visor::ui32> indices;

	Visor::Mesh mesh(vertices, indices, "../../../assets/shaders/intermediate/vertex.spv", "../../../assets/shaders/intermediate/fragment.spv");

	Visor::Entity entity({ 0.0f, 0.0f, 3.0f }, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, mesh);
	entities.push_back(entity);
	
	Visor::Renderer::start(window, entities);

	while (!window.shouldClose())
	{
		window.pollEvents();
		
		camera.yaw += 0.0001f;

		Visor::Renderer::getInstance().render(camera);
	}

	Visor::Renderer::terminate();

	window.close();

	return 0;
}