#pragma once
#include "GLUtils.hpp"

template <typename SurfT>
[[nodiscard]] MeshObject<Vertex> GetParamSurfMesh(const SurfT &surf, const std::size_t N = 80, const std::size_t M = 40)
{
	MeshObject<Vertex> outputMesh;

	// We approximate the parametric surface with NxM quadrilaterals, so we need to evaluate at (N+1)x(M+1) points
	outputMesh.vertexArray.resize((N + 1) * (M + 1));

	for (std::size_t j = 0; j <= M; ++j)
	{
		for (std::size_t i = 0; i <= N; ++i)
		{
			float u = i / (float)N;
			float v = j / (float)M;

			std::size_t index = i + j * (N + 1);
			outputMesh.vertexArray[index].position = surf.GetPos(u, v);
			outputMesh.vertexArray[index].normal = surf.GetNorm(u, v);
			outputMesh.vertexArray[index].texcoord = surf.GetTex(u, v);
		}
	}

	// Index buffer data: NxM quadrilaterals = 2xNxM triangles = for triangle lists 3x2xNxM indices
	outputMesh.indexArray.resize(3 * 2 * (N) * (M));

	for (std::size_t j = 0; j < M; ++j)
	{
		for (std::size_t i = 0; i < N; ++i)
		{
			// Each quadrilateral is divided into two triangles connecting points for (u_i, v_j) parameter values at (i,j) indices
			// (i,j+1) C-----D (i+1,j+1)
			//         |\    |              A = p(u_i, v_j)
			//         | \   |              B = p(u_{i+1}, v_j)
			//         |  \  |              C = p(u_i, v_{j+1})
			//         |   \ |              D = p(u_{i+1}, v_{j+1})
			//         |    \|
			//   (i,j) A-----B (i+1,j)
			// - The 1D index for (i,j) in the VBO: i+j*(N+1)
			// - The 1D index for (i,j) in the IB: i*6+j*6*N
			//      (because each quadrilateral contains 2 triangles = 6 indices)
			std::size_t index = i * 6 + j * (6 * N);
			outputMesh.indexArray[index + 0] = static_cast<GLuint>((i) + (j) * (N + 1));
			outputMesh.indexArray[index + 1] = static_cast<GLuint>((i + 1) + (j) * (N + 1));
			outputMesh.indexArray[index + 2] = static_cast<GLuint>((i) + (j + 1) * (N + 1));
			outputMesh.indexArray[index + 3] = static_cast<GLuint>((i + 1) + (j) * (N + 1));
			outputMesh.indexArray[index + 4] = static_cast<GLuint>((i + 1) + (j + 1) * (N + 1));
			outputMesh.indexArray[index + 5] = static_cast<GLuint>((i) + (j + 1) * (N + 1));
		}
	}

	return outputMesh;
}
