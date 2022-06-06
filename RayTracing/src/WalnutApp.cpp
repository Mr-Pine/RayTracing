#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include <iostream>
#include <bitset>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace Walnut;
using namespace glm;

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render")) {
			Render();
		}
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::SliderFloat3("Camera position", &m_CameraPosition.x, -5, 5);
			ImGui::SliderFloat3("Camera rotation", &m_CameraRotation.x, -pi, pi);
			if (ImGui::CollapsingHeader("Viewport")) {
				ImGui::SliderFloat("Viewport distance", &m_ViewportDistance, 0, 5);
				ImGui::SliderFloat("Viewport height", &m_ViewportCameraHeight, 0, 5);
			}
			if (ImGui::CollapsingHeader("Sphere")) {
				ImGui::SliderFloat3("Sphere position", &sphereOrigin.x, -5, 5);
				ImGui::SliderFloat("Sphere radius", &sphereRadius, 0, 5);
			}
			if (ImGui::CollapsingHeader("Light")) {
				ImGui::SliderFloat3("Light direction", &m_LightDirection.x, -1, 1);
			}
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		if (m_Image) {
			ImGui::Image(m_Image->GetDescriptorSet(), { (float)m_Image->GetWidth(), (float)m_Image->GetHeight() });
		}

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	void Render() {

		Timer timer;

		if (!m_Image || m_ViewportWidth != m_Image->GetWidth() || m_ViewportHeight != m_Image->GetHeight()) {
			m_Image = std::make_shared<Image>(m_ViewportWidth, m_ViewportHeight, ImageFormat::RGBA);
			delete[] m_ImageData;
			m_ImageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];
		}

		if (m_CameraRotationCache != m_CameraRotation) {
			m_RotationValues = { {sin(-m_CameraRotation.x), cos(-m_CameraRotation.x)}, {sin(m_CameraRotation.y), cos(m_CameraRotation.y)}, {sin(m_CameraRotation.z), cos(m_CameraRotation.z)} };
			m_CameraUp = normalize(rotate3d(vec3(0, 1, 0), m_RotationValues));
			m_CameraRight = normalize(rotate3d(vec3(1, 0, 0), m_RotationValues));
			m_CameraForward = normalize(rotate3d(vec3(0, 0, 1), m_RotationValues)) * m_ViewportDistance;
			m_CameraRotationCache = m_CameraRotation;
		}

		for (uint32_t i = 0; i < m_ViewportWidth * m_ViewportHeight; i++) {

			int x = i % m_ViewportWidth;
			int y = (int)(i / m_ViewportWidth);

			float directionX = (m_ViewportCameraHeight / m_ViewportHeight) * (x - (float)m_ViewportWidth / 2);
			float directionY = -m_ViewportCameraHeight * (y - (float)m_ViewportHeight / 2) / m_ViewportHeight;


			vec3 rayDirection = directionX * m_CameraRight + directionY * m_CameraUp + m_CameraForward;
			rayDirection = normalize(rayDirection);


			float a = 1;//dot(rayDirection, rayDirection);
			float b = 2 * (dot(m_CameraPosition, rayDirection) - dot(sphereOrigin, rayDirection));
			float c = dot(m_CameraPosition, m_CameraPosition) + dot(sphereOrigin, sphereOrigin) - 2 * dot(m_CameraPosition, sphereOrigin) - sphereRadius * sphereRadius;

			float discriminant = (b * b) - (4 * a * c);

			if (discriminant > 0.0f) {
				float hitDistance = min((-b + sqrt(discriminant)), (-b - sqrt(discriminant))) * 0.5;//= / (2 * a);
				if (hitDistance > 0) {
					vec3 hitPoint = m_CameraPosition + rayDirection * hitDistance;
					vec3 normal = normalize(hitPoint - sphereOrigin);
					float angle = -dot(normal, normalize(m_LightDirection));
					int32_t mappedDistance = (int)((3 - hitDistance) * 256) << 16;
					m_ImageData[i] = /*mappedDistance |*/ 0xff990000;
					int32_t lighting = (int32_t)(angle * (angle > 0) * 256) << 8;
					m_ImageData[i] |= lighting;

					continue;
				}
			}


			m_ImageData[i] = 0xff000000;
		}

		m_Image->SetData(m_ImageData);

		m_LastRenderTime = timer.ElapsedMillis();
	}
private:
	const float pi = 3.1415926535897932384626433832795;

	std::shared_ptr<Image> m_Image;
	uint32_t m_ViewportWidth = 8, m_ViewportHeight = 0;
	uint32_t* m_ImageData = nullptr;
	float m_LastRenderTime = 0.0f;

	vec3 sphereOrigin = vec3(0, 0, 0);
	float sphereRadius = 1.0f;

	vec3 m_CameraPosition = vec3(0.0f, 0.0f, -5.0f);
	vec3 m_CameraRotation = vec3(0, 0, 0);
	vec3 m_CameraRotationCache = m_CameraRotation + vec3(1,1,1);
	std::vector<std::vector<float>> m_RotationValues = { {0, 0}, {0, 0}, {0, 0} };
	float m_ViewportCameraHeight = 2;
	float m_ViewportDistance = 2;

	vec3 m_CameraUp;
	vec3 m_CameraRight;
	vec3 m_CameraForward;

	vec3 m_LightDirection = vec3(1, -1, 1);

	vec3 rotate3d(vec3 vector, std::vector<std::vector<float>> values) {
		return rotate3d(vector, values[0], values[1], values[2]);
	}

	vec3 rotate3d(vec3 vector, std::vector<float> valuesX, std::vector<float> valuesY, std::vector<float> valuesZ) {
		#define Sin 0
		#define Cos 1
		
		vec3 result = vector;

		/*result.x = vector.x * valuesZ[Cos] - vector.y * valuesZ[Sin];
		result.y = vector.x * valuesZ[Sin] + vector.y * valuesZ[Cos];

		result.x = result.x * valuesY[Cos] + result.z * valuesY[Sin];
		result.z = -result.x * valuesY[Sin] + result.z * valuesY[Cos];

		result.y = result.y * valuesX[Cos] - result.z * valuesX[Sin];
		result.z = result.y * valuesX[Sin] + result.z * valuesX[Cos];*/

		result.x = vector.x * (valuesY[Cos] * valuesZ[Cos]) + vector.y * (valuesX[Sin] * valuesY[Sin] * valuesZ[Cos] - valuesX[Cos] * valuesZ[Sin]) + vector.z * (valuesX[Cos] * valuesY[Sin] * valuesZ[Cos] + valuesX[Sin] * valuesZ[Sin]);
		result.y = vector.x * (valuesY[Cos] * valuesZ[Sin]) + vector.y * (valuesX[Sin] * valuesY[Sin] * valuesZ[Sin] + valuesX[Cos] * valuesZ[Cos]) + vector.z * (valuesX[Cos] * valuesY[Sin] * valuesZ[Sin] - valuesX[Sin] * valuesZ[Cos]);
		result.z = vector.x * (-valuesY[Sin]) + vector.y * (valuesX[Sin] * valuesY[Cos]) + vector.z * (valuesX[Cos] * valuesY[Cos]);

		return result;
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "RayTracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					app->Close();
				}
				ImGui::EndMenu();
			}
		});
	return app;
}