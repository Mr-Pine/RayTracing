#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Material
{
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec3 EmissionColor { 0.0f };
	float EmissionPower;
	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; };
};

struct Triangle {
	struct VertexPositions
	{
		glm::vec3 a;
		glm::vec3 b;
		glm::vec3 c;

		glm::vec3 const& operator[] (int i) const;
	};

	VertexPositions Vertices;
	glm::vec3 Normal {0.0f, 0.0f, 0.0f};

	int MaterialIndex;
};

struct Sphere {
	glm::vec3 Position{0.0f, 0.0f, 0.0f};
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct Scene {
	std::vector<Sphere> Spheres;
	std::vector<Triangle> Triangles;
	std::vector<Material> Materials;
};