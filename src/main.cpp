#include <visor.h>

#include <iostream>
#include <vector>

int main()
{
	Visor::Window window(1500, 1200, "test");
	window.open();

	Visor::Renderer::start(window);

	Visor::Camera camera = {};
	std::vector<Visor::Entity> entities;

	while (!window.shouldClose())
	{
		window.pollEvents();
		
		Visor::Renderer::getInstance().render(camera, entities);
	}

	Visor::Renderer::terminate();

	window.close();

	return 0;
}