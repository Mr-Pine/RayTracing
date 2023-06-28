#include <crtdbg.h>
#include <execution>
#include "Walnut/Random.h"

#include "Renderer.h"
#include "Camera.h"


namespace Utils {

	// ReSharper disable once CppInconsistentNaming
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		const uint8_t alpha = (uint8_t)(color.a * 255.0f);
		const uint8_t red = (uint8_t)(color.r * 255.0f);
		const uint8_t green = (uint8_t)(color.g * 255.0f);
		const uint8_t blue = (uint8_t)(color.b * 255.0f);
		return (alpha << 24) | (blue << 16) | (green << 8) | red;
	}

	static uint32_t PCG_Hash(uint32_t input) {
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277903737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed) {
		seed = PCG_Hash(seed);
		return (float)seed / (float)UINT32_MAX;
	}

	static glm::vec3 InUnitSphere(uint32_t& seed) {
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f
		));
	}

}

void Renderer::Render(Scene& scene, const Camera& camera) {
	m_ActiveCamera = &camera;
	m_ActiveScene = &scene;

	const uint32_t width = m_FinalImage->GetWidth();
	const uint32_t height = m_FinalImage->GetHeight();

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, width * height * sizeof(glm::vec4));
#define MT
#ifdef MT
	std::for_each(std::execution::par, m_ImageVertiacalIter.begin(), m_ImageVertiacalIter.end(),
		[this, width, height](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
			[this, y, width, height](uint32_t x)
				{
					glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
					coord = coord * 2.0f - 1.0f; // Map 0 -> 1 to -1 -> 1


					const glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * width] += color;

					glm::vec4 accumulated = m_AccumulationData[x + y * width];
					accumulated /= (float)m_FrameIndex;

					accumulated = glm::clamp(accumulated, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * width] = Utils::ConvertToRGBA(accumulated);
				});
		});
#else
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++) {
			glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
			coord = coord * 2.0f - 1.0f; // Map 0 -> 1 to -1 -> 1


			const glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * width] += color;

			glm::vec4 accumulated = m_AccumulationData[x + y * width];
			accumulated /= (float)m_FrameIndex;

			accumulated = glm::clamp(accumulated, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * width] = Utils::ConvertToRGBA(accumulated);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 light(0);
	glm::vec3 contribution(1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	int bounces = 3;
	for (int i = 0; i < bounces; i++)
	{
		const HitPayload payload = TraceRay(ray);

		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor(0.6f, 0.7f, 0.9f);
			//light += skyColor * contribution;
			break;
		}

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		contribution *= material.Albedo;
		light += material.GetEmission();

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;

		if (!m_Settings.Fast_Random) {
			ray.Direction = glm::normalize(payload.WorldNormal + Walnut::Random::InUnitSphere());
		}
		else {
			ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
		}
	}

	return { light, 1 };
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
	if (m_FinalImage) {
		if (width == m_FinalImage->GetWidth() && height == m_FinalImage->GetHeight()) {
			return; // No resize necessary
		}
		_RPT0(_CRT_WARN, "Updataing image size");
		m_FinalImage->Resize(width, height);
	}
	else {
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);

	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];
	m_FrameIndex = 1;

	m_ImageHorizontalIter.resize(width);
	m_ImageVertiacalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVertiacalIter[i] = i;
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray) {
	float hitDistance = std::numeric_limits<float>::max();
	int closestSphereIndex = -1;
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		const float a = dot(ray.Direction, ray.Direction);
		const float b = 2.0f * dot(origin, ray.Direction);
		const float c = dot(origin, origin) - sphere.Radius * sphere.Radius;

		const float discriminant = (b * b) - (4 * a * c);

		if (discriminant < 0.0f)
			continue;

		const float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance) {
			closestSphereIndex = i;
			hitDistance = closestT;
		}
	}
#define RENDER_TRIS
#ifdef RENDER_TRIS
	int closestTriIndex = -1;
	for (size_t i = 0; i < m_ActiveScene->Triangles.size(); i++) {
		Triangle& triangle = m_ActiveScene->Triangles[i];

		if (triangle.Normal == glm::vec3(0)) {
			// calculate Normal
			glm::vec3 a = triangle.Vertices[0] - triangle.Vertices[1];
			glm::vec3 b = triangle.Vertices[0] - triangle.Vertices[2];

			glm::vec3 orthogonal = glm::cross(a, b);

			triangle.Normal = orthogonal;
		}

		if (glm::dot(ray.Direction, triangle.Normal) == 0) {
			continue;
		}

		// Do actual ray intersection with triangle
	}
#endif // RENDER_TRIS


	if (closestSphereIndex < 0) {
		return Miss(ray);
	}

	return ClosestHit(ray, hitDistance, closestSphereIndex);


}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	const glm::vec3 origin = ray.Origin - closestSphere.Position;

	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	HitPayload payload;
	payload.HitDistance = -1;
	return payload;
}
