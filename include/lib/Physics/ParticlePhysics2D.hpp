#pragma once
#include "Particle2D.hpp"
#include "ParticleGrid2D.hpp"
using namespace EvoEngine;
namespace EcoSysLab {
	template<typename T>
	class ParticlePhysics2D
	{
		ParticleGrid2D m_particleGrid2D{};
		std::vector<Particle2D<T>> m_particles2D{};
		void SolveContact(ParticleHandle p1Handle, ParticleHandle p2Handle);
		float m_deltaTime = 0.002f;
		void Update(const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc);
		void InitializeGrid();
		void CheckCollisions();
		void CheckCellCollisions(const ParticleCell& cell1, const ParticleCell& cell2);
		glm::vec2 m_min, m_max;
	public:
		float m_particleRadius = 1.0f;
		[[nodiscard]] ParticleHandle AllocateParticle();
		[[nodiscard]] Particle2D<T>& RefParticle(ParticleHandle handle);
		void RemoveParticle(ParticleHandle handle);
		void Shift(const glm::vec2& offset);
		[[nodiscard]] const std::vector<Particle2D<T>>& PeekParticles() const;
		[[nodiscard]] std::vector<Particle2D<T>>& RefParticles();
		void Simulate(float time, const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc);

		void OnInspect(const std::function<void(glm::vec2 position)>& func, const std::function<void(ImVec2 origin, float zoomFactor, ImDrawList*)>& drawFunc);
	};

	template <typename T>
	void ParticlePhysics2D<T>::SolveContact(ParticleHandle p1Handle, ParticleHandle p2Handle)
	{
		if(p1Handle == p2Handle) return;
		auto& p1 = m_particles2D.at(p1Handle);
		auto& p2 = m_particles2D.at(p2Handle);
		const auto difference = p1.m_position - p2.m_position;
		const auto distance = glm::length(difference);
		const auto minDistance = 2.0f * m_particleRadius;
		if (distance < minDistance)
		{
			const auto axis = distance < glm::epsilon<float>() ? glm::vec2(1, 0) : difference / distance;
			const auto delta = minDistance - distance;
			p1.m_position += 0.5f * delta * axis;
			p2.m_position -= 0.5f * delta * axis;
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::Update(
		const std::function<void(Particle2D<T>& collisionParticle)>& modifyParticleFunc)
	{
		if(m_particles2D.empty()) return;
		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i)
			{
				modifyParticleFunc(m_particles2D[i]);
			}
		);
		InitializeGrid();
		CheckCollisions();
		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i)
			{
				m_particles2D[i].Update(m_deltaTime);
			}
		);
	}

	template <typename T>
	void ParticlePhysics2D<T>::InitializeGrid()
	{
		m_min = glm::vec2(FLT_MAX);
		m_max = glm::vec2(FLT_MIN);

		const auto threadCount = Jobs::Workers().Size();
		std::vector<glm::vec2> mins{};
		std::vector<glm::vec2> maxs{};
		mins.resize(threadCount);
		maxs.resize(threadCount);
		for (int i = 0; i < threadCount; i++)
		{
			mins[i] = glm::vec2(FLT_MAX);
			maxs[i] = glm::vec2(FLT_MIN);
		}
		Jobs::ParallelFor(m_particles2D.size(), [&](const unsigned i, const unsigned threadIndex)
			{
				const auto& particle = m_particles2D[i];
				mins[threadIndex] = glm::min(mins[threadIndex], particle.m_position);
				maxs[threadIndex] = glm::max(maxs[threadIndex], particle.m_position);
			}
		);
		for (int i = 0; i < threadCount; i++)
		{
			m_min = glm::min(mins[i], m_min);
			m_max = glm::max(maxs[i], m_max);
		}
		const auto gridSize = m_max - m_min;
		m_particleGrid2D.Reset(
			static_cast<int>(glm::ceil(gridSize.x / m_particleRadius * 0.5f)) + 1, 
			static_cast<int>(glm::ceil(gridSize.y / m_particleRadius * 0.5f)) + 1);
		for (ParticleHandle i = 0; i < m_particles2D.size(); i++)
		{
			const auto& particle = m_particles2D[i];
			m_particleGrid2D.RegisterParticle(
				static_cast<int>((particle.m_position.x - m_min.x) / m_particleRadius * 0.5f), 
				static_cast<int>((particle.m_position.y - m_min.y) / m_particleRadius * 0.5f), 
				i);
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::CheckCollisions()
	{
		for(int i = 0; i < m_particleGrid2D.m_width - 1; i++)
		{
			for(int j = 0; j < m_particleGrid2D.m_height - 1; j++)
			{
				auto& currentCell = m_particleGrid2D.RefCell(i, j);
				for(int dx = 0; dx <= 1; dx++)
				{
					for (int dy = 0; dy <= 1; dy++)
					{
						auto& otherCell = m_particleGrid2D.RefCell(i + dx, j + dy);
						CheckCellCollisions(currentCell, otherCell);
					}
				}
			}
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::CheckCellCollisions(const ParticleCell& cell1, const ParticleCell& cell2)
	{
		for(size_t i = 0; i < cell1.m_atomCount; i++)
		{
			for (size_t j = 0; j < cell2.m_atomCount; j++)
			{
				const auto h1 = cell1.m_atomHandles[i];
				const auto h2 = cell2.m_atomHandles[j];
				SolveContact(h1, h2);
			}
		}
	}

	template <typename T>
	ParticleHandle ParticlePhysics2D<T>::AllocateParticle()
	{
		m_particles2D.emplace_back();
		return m_particles2D.size() - 1;
	}

	template <typename T>
	Particle2D<T>& ParticlePhysics2D<T>::RefParticle(ParticleHandle handle)
	{
		return m_particles2D[handle];
	}

	template <typename T>
	void ParticlePhysics2D<T>::RemoveParticle(ParticleHandle handle)
	{
		m_particles2D[handle] = m_particles2D.back();
		m_particles2D.pop_back();
	}

	template <typename T>
	void ParticlePhysics2D<T>::Shift(const glm::vec2& offset)
	{
		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i)
			{
				auto& particle = m_particles2D[i];
				particle.SetPosition(particle.m_position + offset);
			}
		);
	}

	template <typename T>
	const std::vector<Particle2D<T>>& ParticlePhysics2D<T>::PeekParticles() const
	{
		return m_particles2D;
	}

	template <typename T>
	std::vector<Particle2D<T>>& ParticlePhysics2D<T>::RefParticles()
	{
		return m_particles2D;
	}

	template <typename T>
	void ParticlePhysics2D<T>::Simulate(float time,
		const std::function<void(Particle2D<T>& collisionParticle)>& modifyParticleFunc)
	{
		const auto count = static_cast<size_t>(glm::round(time / m_deltaTime));
		for (size_t i{ count }; i--;)
		{
			Update(modifyParticleFunc);
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::OnInspect(const std::function<void(glm::vec2 position)>& func, const std::function<void(ImVec2 origin, float zoomFactor, ImDrawList*)>& drawFunc)
	{
		static auto scrolling = glm::vec2(0.0f);
		static float zoomFactor = 1.f;
		if (ImGui::Button("Recenter")) {
			scrolling = glm::vec2(0.0f);
		}
		ImGui::DragFloat("Zoom", &zoomFactor, zoomFactor / 100.0f, 0.01f, 50.0f);
		zoomFactor = glm::clamp(zoomFactor, 0.01f, 50.0f);
		const ImGuiIO& io = ImGui::GetIO();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const ImVec2 canvasP0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
		ImVec2 canvasSz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
		if (canvasSz.x < 50.0f) canvasSz.x = 50.0f;
		if (canvasSz.y < 50.0f) canvasSz.y = 50.0f;
		const ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);
		const ImVec2 origin(canvasP0.x + canvasSz.x / 2.0f + scrolling.x,
			canvasP0.y + canvasSz.y / 2.0f + scrolling.y); // Lock scrolled origin
		const ImVec2 mousePosInCanvas((io.MousePos.x - origin.x) / zoomFactor,
			(io.MousePos.y - origin.y) / zoomFactor);

		// Draw border and background color
		drawList->AddRectFilled(canvasP0, canvasP1, IM_COL32(50, 50, 50, 255));
		drawList->AddRect(canvasP0, canvasP1, IM_COL32(255, 255, 255, 255));

		// This will catch our interactions
		ImGui::InvisibleButton("canvas", canvasSz,
			ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool isMouseHovered = ImGui::IsItemHovered(); // Hovered
		const bool isMouseActive = ImGui::IsItemActive();   // Held

		// Pan (we use a zero mouse threshold when there's no context menu)
		// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
		const float mouseThresholdForPan = -1.0f;
		if (isMouseActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouseThresholdForPan)) {
			scrolling.x += io.MouseDelta.x;
			scrolling.y += io.MouseDelta.y;
		}
		// Context menu (under default mouse threshold)
		const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
		if (dragDelta.x == 0.0f && dragDelta.y == 0.0f)
			ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
		if (ImGui::BeginPopup("context")) {

			ImGui::EndPopup();
		}

		// Draw profile + all lines in the canvas
		drawList->PushClipRect(canvasP0, canvasP1, true);
		if (isMouseHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			func(glm::vec2(mousePosInCanvas.x, mousePosInCanvas.y));
		}
		for (const auto& particle : m_particles2D) {
			const auto& pointPosition = particle.m_position;
			const auto& pointRadius = m_particleRadius;
			const auto& pointColor = particle.m_color;
			const auto canvasPosition = ImVec2(origin.x + pointPosition.x * zoomFactor,
				origin.y + pointPosition.y * zoomFactor);

			drawList->AddCircleFilled(canvasPosition,
				glm::clamp(zoomFactor * pointRadius, 1.0f, 100.0f),
				IM_COL32(255.0f * pointColor.x, 255.0f * pointColor.y, 255.0f * pointColor.z, 255.0f * pointColor.w));
		}

		drawList->AddCircle(origin,
			glm::clamp(0.5f * zoomFactor, 1.0f, 100.0f),
			IM_COL32(255,
				0,
				0, 255));
		for (int i = 0; i < m_particleGrid2D.m_width; i++)
		{
			for (int j = 0; j < m_particleGrid2D.m_height; j++)
			{
				ImVec2 min = ImVec2(m_min.x + i * 2.0f * m_particleRadius, m_min.y + j * 2.0f * m_particleRadius);
				drawList->AddQuad(
					min * zoomFactor + origin, 
					ImVec2(min.x + 2.0f * m_particleRadius, min.y) * zoomFactor + origin,
					ImVec2(min.x + 2.0f * m_particleRadius, min.y + 2.0f * m_particleRadius) * zoomFactor + origin,
					ImVec2(min.x, min.y + 2.0f * m_particleRadius) * zoomFactor + origin,
					IM_COL32(0, 255, 0, 255));
			}
		}
		drawFunc(origin, zoomFactor, drawList);
		drawList->PopClipRect();

	}
}