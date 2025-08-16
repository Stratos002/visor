#include "mesh.h"

namespace Visor
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<ui32>& indices)
		: _vertices(vertices)
		, _indices(indices)
	{}

	const std::vector<Mesh::Vertex>& Mesh::getVertices() const
	{
		return _vertices;
	}

	const std::vector<ui32>& Mesh::getIndices() const
	{
		return _indices;
	}
}