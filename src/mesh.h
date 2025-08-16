#pragma once

#include "types.h"
#include "maths.h"

#include <vector>

namespace Visor
{
	class Mesh
	{
	public:
		struct Vertex
		{
			Vector3<f32> position;
		};

		Mesh(const std::vector<Vertex>& vertices, const std::vector<ui32>& indices);
		
		const std::vector<Vertex>& getVertices() const;
		const std::vector<ui32>& getIndices() const;

	private:
		std::vector<Vertex> _vertices;
		std::vector<ui32> _indices;
	};
}
