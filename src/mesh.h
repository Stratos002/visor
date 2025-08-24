#pragma once

#include "types.h"
#include "maths.h"

#include <vector>
#include <string>

namespace Visor
{
	class Mesh
	{
	public:
		struct Vertex
		{
			Vector3<f32> position;
			Vector3<f32> normal;
		};

		Mesh(const std::vector<Vertex>& vertices, const std::vector<ui32>& indices, const std::string& vertexShaderName, const std::string& fragmentShaderName);
		
		const std::vector<Vertex>& getVertices() const;
		const std::vector<ui32>& getIndices() const;
		const std::string& getVertexShaderName() const;
		const std::string& getFragmentShaderName() const;

	private:
		std::vector<Vertex> _vertices;
		std::vector<ui32> _indices;
		std::string _vertexShaderName;
		std::string _fragmentShaderName;
	};
}
