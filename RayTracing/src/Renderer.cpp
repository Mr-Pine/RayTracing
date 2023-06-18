#include <crtdbg.h>
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

}

void Renderer::Render(const Scene& scene, const Camera& camera) {
	m_ActiveCamera = &camera;
	m_ActiveScene = &scene;

	const uint32_t width = m_FinalImage->GetWidth();
	const uint32_t height = m_FinalImage->GetHeight();
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++) {
			glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
			coord = coord * 2.0f - 1.0f; // Map 0 -> 1 to -1 -> 1


			glm::vec4 color = PerPixel(x, y);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * width] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 color(0);
	float multiplier = 1.0f;

	int bounces = 10;
	for (int i = 0; i < bounces; i++)
	{
		const HitPayload payload = TraceRay(ray);

		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor(0.6f, 0.7f, 0.9f);
			color += skyColor * multiplier;
			break;
		}

		const glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));

		const float lightIntensity = glm::max(-glm::dot(payload.WorldNormal, lightDir), 0.0f); // == cos(angle)

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];
		glm::vec3 sphereColor = material.Albedo * lightIntensity;
		color += sphereColor * multiplier;

		multiplier *= 0.7f;


		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, glm::normalize(payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f, 0.5f)));
	}

	return { color, 1 };
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
	if (m_FinalImage) {
		if (width == m_FinalImage->GetWidth() && height == m_FinalImage->GetHeight()) {
			return; // No resize necessary
		}
		else {
			_RPT0(_CRT_WARN, "Updataing image size");
			m_FinalImage->Resize(width, height);
		}
	}
	else {
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);

	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray) {
	int closestSphereIndex = -1;
	float hitDistance = std::numeric_limits<float>::max();
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
