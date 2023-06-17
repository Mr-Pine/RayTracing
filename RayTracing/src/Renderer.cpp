#include <crtdbg.h>
#include "Renderer.h"


namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t alpha = (uint8_t)(color.a * 255.0f);
		uint8_t red = (uint8_t)(color.r * 255.0f);
		uint8_t green = (uint8_t)(color.g * 255.0f);
		uint8_t blue = (uint8_t)(color.b * 255.0f);
		return (alpha << 24) | (blue << 16) | (green << 8) | red;
	}

}

void Renderer::Render(const Scene& scene, const Camera& camera) {
	Ray ray;
	ray.Origin = camera.GetPosition();

	uint32_t width = m_FinalImage->GetWidth();
	uint32_t height = m_FinalImage->GetHeight();
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetHeight(); x++) {
			glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
			coord = coord * 2.0f - 1.0f; // Map 0 -> 1 to -1 -> 1

			ray.Direction = camera.GetRayDirections()[x + y * width];

			glm::vec4 color = TraceRay(scene, ray);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * width] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
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

glm::vec4 Renderer::TraceRay(const Scene& scene, const Ray& ray) {
	if (scene.Spheres.size() == 0) {
		return glm::vec4(0, 0, 0, 1);
	}

	const Sphere* closestSphere = nullptr;
	float hitDistance = std::numeric_limits<float>::max();
	for (const Sphere& sphere : scene.Spheres)
	{
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = (b * b) - (4 * a * c);

		if (discriminant < 0.0f)
			continue;

		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT < hitDistance) {
			closestSphere = &sphere;
			hitDistance = closestT;
		}
	}

	if (closestSphere == nullptr) {
		return glm::vec4(0, 0, 0, 1);
	}

	glm::vec3 origin = ray.Origin - closestSphere->Position;

	glm::vec3 hitPoint = origin + ray.Direction * hitDistance;
	glm::vec3 normal = glm::normalize(hitPoint);

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
	float lightIntensity = glm::max(-glm::dot(normal, lightDir), 0.0f); // == cos(angle)

	glm::vec3 sphereColor = closestSphere->Albedo;
	sphereColor *= lightIntensity;
	return glm::vec4(sphereColor, 1);
}