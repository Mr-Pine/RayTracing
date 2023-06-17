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

void Renderer::Render(const Camera& camera) {
	Ray ray;
	ray.Origin = camera.GetPosition();

	uint32_t width = m_FinalImage->GetWidth();
	uint32_t height = m_FinalImage->GetHeight();
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetHeight(); x++) {
			glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
			coord = coord * 2.0f - 1.0f; // Map 0 -> 1 to -1 -> 1

			ray.Direction = camera.GetRayDirections()[x + y * width];

			glm::vec4 color = TraceRay(ray);
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
		} else {
			m_FinalImage->Resize(width, height);
		}
	}
	else {
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);

	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

glm::vec4 Renderer::TraceRay(const Ray& ray) {
	float radius = 0.5f;

	float a = glm::dot(ray.Direction, ray.Direction);
	float b = 2.0f * glm::dot(ray.Origin, ray.Direction);
	float c = glm::dot(ray.Origin, ray.Origin) - radius * radius;

	float discriminant = (b * b) - (4 * a * c);

	if (discriminant < 0.0f)
		return glm::vec4(0, 0, 0, 1);
	_RPT1(_CRT_WARN, "Discriminant not 0: %f\n", discriminant);

	float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
	float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)

	glm::vec3 hitPoint = ray.Origin + ray.Direction * closestT;
	glm::vec3 normal = glm::normalize(hitPoint);

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
	float lightIntensity = glm::max(-glm::dot(normal, lightDir), 0.0f); // == cos(angle)

	glm::vec3 sphereAccentColor(0, 1, 0);
	sphereAccentColor *= lightIntensity;
	glm::vec3 sphereColor = glm::vec3(0, 0, 1) + sphereAccentColor;

	_RPTN(_CRT_WARN, "Color: r %f, g %f, b %f", sphereColor.r, sphereColor.g, sphereColor.b);

	return glm::vec4(sphereColor, 1);
}