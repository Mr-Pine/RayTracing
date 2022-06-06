#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include <bitset>
#include <iostream>

using namespace Walnut;

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
		ImGui::SliderFloat("Viewport distance", &m_ViewportDistance, 0.2f, 20);

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

		struct Vec3
		{
			float                                   x, y, z;
			Vec3() { x = y = z = 0.0f; }
			Vec3(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
			float  operator[] (size_t idx) const { IM_ASSERT(idx <= 1); return (&x)[idx]; }    // We very rarely use this [] operator, the assert overhead is fine.
			float& operator[] (size_t idx) { IM_ASSERT(idx <= 1); return (&x)[idx]; }    // We very rarely use this [] operator, the assert overhead is fine.
			float operator* (Vec3 other) { return (x * other.x + y * other.y + z * other.z); }
			float operator* (float other) { return (x * other + y * other + z * other); }
			Vec3 normalized() { float length = sqrt(operator*(Vec3(x, y, z))); return Vec3(x / length, y / length, z / length); }
		};

		Vec3 sphereOrigin = Vec3(1,2,0);
		float sphereRadius = 1.0f;

		Vec3 cameraPosition = Vec3(0.0f, 0.0f, 3.0f);
		Vec3 cameraDirection = Vec3(0, 0, -1);
		float cameraTilt = 0; //TODO: implement camera tilt
		float viewportHeight = 2;




		for (uint32_t i = 0; i < m_ViewportWidth * m_ViewportHeight; i++) {

			int x = i % m_ViewportWidth;
			int y = (int)(i / m_ViewportWidth);

			float directionX = -(viewportHeight / m_ViewportHeight) * (x - (float)m_ViewportWidth / 2);
			float directionY = viewportHeight * (y - (float)m_ViewportHeight / 2) / m_ViewportHeight;

			

			Vec3 rayDirection = Vec3(directionX, directionY, -m_ViewportDistance);
			rayDirection = rayDirection.normalized();


			float a = rayDirection * rayDirection;
			float b = 2 * (cameraPosition * rayDirection + sphereOrigin * rayDirection);
			float c = cameraPosition * cameraPosition + sphereOrigin * sphereOrigin - sphereRadius * sphereRadius;

			float discriminant = (b * b) - (4 * a * c);

			if (discriminant < 0.0f) {
				m_ImageData[i] = 0xff000000;
			}
			else {
				float hitDistance = abs((b + sqrt(discriminant)) / (2 * a));
				int32_t mappedDistance = (int)((3 - hitDistance) * 256) << 16;
				m_ImageData[i] = mappedDistance | 0xff000000;
			}

			std::byte xScaled = (std::byte)((float)x / m_ViewportWidth * 256);
			std::byte yScaled = (std::byte)((float)y / m_ViewportHeight * 256);
			//std::bitset<8> bitset{ (uint64_t)xScaled };
			//std::cout << bitset << std::endl;
			//std::cout << std::hex << ((uint32_t)xScaled << 16) << std::dec << std::endl;
			/*m_ImageData[i] = (uint32_t)xScaled << 16 | (uint32_t)yScaled << 8;
			m_ImageData[i] |= 0xff000000;
			*/
			//m_ImageData[i] = Random::UInt();
			//m_ImageData[i] |= 0xff000000;
		}

		m_Image->SetData(m_ImageData);

		m_LastRenderTime = timer.Elapsed();
	}
private:
	std::shared_ptr<Image> m_Image;
	uint32_t m_ViewportWidth = 8, m_ViewportHeight = 0;
	uint32_t* m_ImageData = nullptr;
	float m_LastRenderTime = 0.0f;
	float m_ViewportDistance = 2;
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