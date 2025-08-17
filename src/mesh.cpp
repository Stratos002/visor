#include "mesh.h"

namespace Visor
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<ui32>& indices, const std::string& vertexShaderName, const std::string& fragmentShaderName)
		: _vertices(vertices)
		, _indices(indices)
		, _vertexShaderName(vertexShaderName)
		, _fragmentShaderName(fragmentShaderName)
	{}

	const std::vector<Mesh::Vertex>& Mesh::getVertices() const
	{
		return _vertices;
	}

	const std::vector<ui32>& Mesh::getIndices() const
	{
		return _indices;
	}

	const std::string& Mesh::getVertexShaderName() const
	{
		return _vertexShaderName;
	}

	const std::string& Mesh::getFragmentShaderName() const
	{
		return _fragmentShaderName;
	}
}