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

static void addRandomEntities(const Visor::Mesh& mesh, std::vector<Visor::Entity>& entities)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(-5, 5);

	const Visor::ui32 entityCount = 10;
	const Visor::f32 scale = 0.3f;

	for(Visor::ui32 entityIndex = 0; entityIndex < entityCount; ++entityIndex)
	{
		Visor::Entity entity({(Visor::f32)distrib(gen), (Visor::f32)distrib(gen), (Visor::f32)distrib(gen)}, scale, scale, scale, 0.0f, 0.0f, 0.0f, mesh);	
		entities.push_back(entity);
	}
}

static void updatePlayer(Visor::Entity& player)
{
	Visor::f32 speed = 0.0f;

	if(Visor::InputSystem::getInstance().isKeyPressed(Visor::InputSystem::Key::W))
	{
		speed += 0.02f;
	}
	if(Visor::InputSystem::getInstance().isKeyPressed(Visor::InputSystem::Key::S))
	{
		speed -= 0.02f;
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

	player.yaw += yaw;
	player.pitch += pitch;
	
	Visor::Vector3<Visor::f32> forward = {
			0.0f,
			0.0f,
			1.0f,
	};
	const Visor::Matrix3<Visor::f32> rotation = (Visor::Matrix4<Visor::f32>::getRotation(player.yaw, player.pitch, 0.0f)).getUpperLeft();
	const Visor::Vector3<Visor::f32> rotatedForward = rotation * forward;
	const Visor::Vector3<Visor::f32> scaledRotatedForwardVector = rotatedForward * speed;
	
	player.position = player.position + scaledRotatedForwardVector;
}

static void updateOtherEntities(std::vector<Visor::Entity>& entities)
{
	static Visor::f32 toy = 0.0f;
	for(Visor::ui32 i = 2; i < entities.size(); ++i)
	{
		Visor::Vector3<Visor::f32> direction = entities[0].position - entities[i].position;
		direction.normalize();

		Visor::f32 yaw = 0.0f;
		Visor::f32 pitch = 0.0f;
		Visor::f32 roll = 0.0f;

		Visor::getAnglesFromDirection(direction, yaw, pitch, roll);

		entities[i].yaw = yaw;
		entities[i].pitch = pitch;
		entities[i].roll = roll;
		entities[i].scaleX = std::abs(std::cosf(toy)) * 0.3f + 0.3f;
		entities[i].scaleZ = std::abs(std::cosf(toy)) * 0.3f + 0.3f;
	}
	toy += 0.01f;
}

static void updateCamera(const Visor::Entity& player, Visor::Camera& camera)
{
	Visor::Vector3<Visor::f32> playerViewDir = {
			0.0f,
			0.0f,
			1.0f,
	};

	const Visor::Matrix4<Visor::f32> translation = Visor::Matrix4<Visor::f32>::getTranslation(player.position);
	const Visor::Matrix4<Visor::f32> rotation = Visor::Matrix4<Visor::f32>::getRotation(player.yaw, player.pitch, 0.0f);
	const Visor::Vector3<Visor::f32> rotatedForward = rotation.getUpperLeft() * playerViewDir;

	Visor::Vector4<Visor::f32> cameraPosition = {1.5f, 1.5f, -5.0f, 1.0f};
	cameraPosition = translation * rotation * cameraPosition;
	
	camera.position.x = cameraPosition.x;
	camera.position.y = cameraPosition.y;
	camera.position.z = cameraPosition.z;

	camera.yaw = player.yaw;
	camera.pitch = player.pitch;
}

static Visor::Mesh getAABBMesh(const Visor::AABB& AABB)
{
	const Visor::Vector3<Visor::f32> widths = AABB.maximum - AABB.minimum;

	const Visor::Vector3<Visor::f32> positions[8] = {
		AABB.minimum,
		AABB.minimum + Visor::Vector3<Visor::f32>{widths.x, 0.0f, 0.0f},
		AABB.minimum + Visor::Vector3<Visor::f32>{0.0f, widths.y, 0.0f},
		AABB.minimum + Visor::Vector3<Visor::f32>{0.0f, 0.0f, widths.z},
		AABB.minimum + Visor::Vector3<Visor::f32>{widths.x, widths.y, 0.0f},
		AABB.minimum + Visor::Vector3<Visor::f32>{widths.x, 0.0f, widths.z},
		AABB.minimum + Visor::Vector3<Visor::f32>{0.0f, widths.y, widths.z},
		AABB.maximum
	};

	const Visor::Vector3<Visor::f32> normals[8] = {
		Visor::Vector3<Visor::f32>{-1.0, -1.0, -1.0},
		Visor::Vector3<Visor::f32>{1.0, -1.0, -1.0},
		Visor::Vector3<Visor::f32>{-1.0, 1.0, -1.0},
		Visor::Vector3<Visor::f32>{-1.0, -1.0, 1.0},
		Visor::Vector3<Visor::f32>{1.0, 1.0, -1.0},
		Visor::Vector3<Visor::f32>{1.0, -1.0, 1.0},
		Visor::Vector3<Visor::f32>{-1.0, 1.0, 1.0},
		Visor::Vector3<Visor::f32>{1.0, 1.0, 1.0},
	};

	const std::vector<Visor::Mesh::Vertex> vertices = {
		{positions[0], normals[0]},
		{positions[1], normals[1]},
		{positions[2], normals[2]},
		{positions[3], normals[3]},
		{positions[4], normals[4]},
		{positions[5], normals[5]},
		{positions[6], normals[6]},
		{positions[7], normals[7]},
	};

	const std::vector<Visor::ui32> indices = {
		0, 1, 4, 0, 4, 2,
		1, 7, 4, 1, 5, 7,
		5, 6, 7, 5, 3, 6,
		3, 2, 6, 3, 0, 2,
		2, 4, 7, 2, 7, 6,
		0, 5, 1, 0, 3, 5
	};

	return Visor::Mesh(vertices, indices, "../assets/shaders/intermediate/vertex.spv", "../assets/shaders/intermediate/fragment.spv");
}

int main()
{
	std::vector<Visor::Entity> entities;
	
	const Visor::Mesh cubeMesh = loadMesh("../assets/models/cube.obj");
	const Visor::Mesh manMesh = loadMesh("../assets/models/man.obj");

	Visor::AABB playerAABB({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
	Visor::Entity playerAABBEntity(playerAABB.minimum, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, getAABBMesh(playerAABB));
	entities.push_back(playerAABBEntity);
	
	Visor::AABB targetAABB({1.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 1.0f});
	Visor::Entity targetAABBEntity(targetAABB.minimum, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, getAABBMesh(targetAABB));
	entities.push_back(targetAABBEntity);
	
	//addRandomEntities(manMesh, entities);
	
	Visor::InputSystem::start();
	Visor::WindowSystem::start(1000, 700);
	Visor::RenderSystem::start();
	
	Visor::Camera camera = {};
	camera.fov = 1.2f;
	
	Visor::Ray ray({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	
	while(!Visor::WindowSystem::getInstance().getWindow().shouldClose())
	{
		Visor::InputSystem::getInstance().update();
		Visor::WindowSystem::getInstance().pollEvents();

		updatePlayer(entities[0]);
		updateOtherEntities(entities);
		updateCamera(entities[0], camera);

		ray.position = entities[0].position;
		ray.direction = Visor::getDirectionFromAngles(entities[0].yaw, entities[0].pitch, entities[0].roll);

		Visor::Vector3<Visor::f32> intersection;
		if(ray.getIntersection(targetAABB, intersection))
		{
			std::cout << "hit !\n";
		}
		else
		{
			std::cout << "miss\n";
		}
		
		Visor::RenderSystem::getInstance().render(camera, entities);
	}

	Visor::RenderSystem::terminate();
	Visor::WindowSystem::terminate();
	Visor::InputSystem::terminate();

	return 0;
}