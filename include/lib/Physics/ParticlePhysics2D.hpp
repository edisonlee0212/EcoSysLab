#pragma once
#include "Particle2D.hpp"
#include "ParticleGrid2D.hpp"
#include "Times.hpp"
using namespace EvoEngine;
namespace EcoSysLab {
	template<typename T>
	class ParticlePhysics2D
	{
		ParticleGrid2D m_particleGrid2D{};
		std::vector<Particle2D<T>> m_particles2D{};
		void SolveCollision(ParticleHandle p1Handle, ParticleHandle p2Handle);
		float m_deltaTime = 0.002f;
		void Update(const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc);
		void InitializeGrid();
		void CheckCollisions();
		glm::vec2 m_min = glm::vec2(FLT_MAX);
		glm::vec2 m_max = glm::vec2(FLT_MIN);
		glm::vec2 m_massCenter = glm::vec2(0.0f);
		float m_totalMoveDelta = 0.0f;
		float m_activeness = 0.0f;
		float m_simulationTime = 0.0f;
	public:
		[[nodiscard]] float GetDistanceToCenter(const glm::vec2& direction);
		[[nodiscard]] float GetTotalMoveDelta() const;
		[[nodiscard]] float GetDeltaTime() const;
		void Reset(float deltaTime = 0.002f);
		void CalculateMinMax();
		float m_particleRadius = 0.01f;
		float m_particleSoftness = 0.5f;
		[[nodiscard]] ParticleHandle AllocateParticle();
		[[nodiscard]] Particle2D<T>& RefParticle(ParticleHandle handle);
		void RemoveParticle(ParticleHandle handle);
		void Shift(const glm::vec2& offset);
		[[nodiscard]] const std::vector<Particle2D<T>>& PeekParticles() const;
		[[nodiscard]] std::vector<Particle2D<T>>& RefParticles();
		void SimulateByTime(float time, const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc);
		void Simulate(size_t iterations, const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc);
		[[nodiscard]] glm::vec2 GetMassCenter() const;
		void OnInspect(const std::function<void(glm::vec2 position)>& func, const std::function<void(ImVec2 origin, float zoomFactor, ImDrawList*)>& drawFunc, bool showGrid = false);
	};

	template <typename T>
	void ParticlePhysics2D<T>::SolveCollision(ParticleHandle p1Handle, ParticleHandle p2Handle)
	{
		auto& p1 = m_particles2D.at(p1Handle);
		const auto& p2 = m_particles2D.at(p2Handle);
		const auto difference = p1.m_position - p2.m_position;
		const auto distance = glm::length(difference);
		const auto minDistance = 2.0f * m_particleRadius;
		if (distance < minDistance)
		{
			const auto axis = distance < glm::epsilon<float>() ? p1Handle >= p2Handle ? glm::vec2(1, 0) : glm::vec2(0, 1) : difference / distance;
			const auto delta = minDistance - distance;
			p1.m_deltaPosition += m_particleSoftness * 0.5f * delta * axis;
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::Update(
		const std::function<void(Particle2D<T>& collisionParticle)>& modifyParticleFunc)
	{
		if (m_particles2D.empty()) return;
		auto currentTime = Times::Now();
		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i)
			{
				auto& particle = m_particles2D[i];
				modifyParticleFunc(particle);
				particle.m_deltaPosition = glm::vec2(0);
			}
		);
		InitializeGrid();
		CheckCollisions();

		const auto threadCount = Jobs::Workers().Size();
		std::vector<float> moveDelta{};
		moveDelta.resize(threadCount);
		std::vector<float> activeness{};
		activeness.resize(threadCount);
		for (int i = 0; i < threadCount; i++)
		{
			moveDelta[i] = 0.0f;
			activeness[i] = 0.0f;
		}

		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i, const unsigned threadIndex)
			{
				auto& particle = m_particles2D[i];
				particle.m_position += particle.m_deltaPosition;
				particle.Update(m_deltaTime);
				moveDelta[threadIndex] += glm::length(particle.GetVelocity(m_deltaTime));
				activeness[threadIndex] += moveDelta[threadIndex] * moveDelta[threadIndex];
			}
		);

		m_simulationTime = Times::Now() - currentTime;
		m_totalMoveDelta = 0.0f;
		for (const auto& d : moveDelta) m_totalMoveDelta += d;
		for (const auto& a : activeness) m_activeness += a;
		m_totalMoveDelta /= m_particles2D.size();
		m_activeness /= m_particles2D.size();
	}

	template <typename T>
	void ParticlePhysics2D<T>::CalculateMinMax()
	{
		m_min = glm::vec2(FLT_MAX);
		m_max = glm::vec2(FLT_MIN);

		const auto threadCount = Jobs::Workers().Size();
		std::vector<glm::vec2> mins{};
		std::vector<glm::vec2> maxs{};
		std::vector<glm::vec2> massCenters{};
		mins.resize(threadCount);
		maxs.resize(threadCount);
		massCenters.resize(threadCount);
		for (int i = 0; i < threadCount; i++)
		{
			mins[i] = glm::vec2(FLT_MAX);
			maxs[i] = glm::vec2(FLT_MIN);
			massCenters[i] = glm::vec2(0.0f);
		}
		Jobs::ParallelFor(m_particles2D.size(), [&](const unsigned i, const unsigned threadIndex)
			{
				const auto& particle = m_particles2D[i];
				mins[threadIndex] = glm::min(mins[threadIndex], particle.m_position);
				maxs[threadIndex] = glm::max(maxs[threadIndex], particle.m_position);

				massCenters[threadIndex] += particle.m_position;
			}
		);

		m_massCenter = glm::vec2(0.0f);
		for (int i = 0; i < threadCount; i++)
		{
			m_min = glm::min(mins[i], m_min);
			m_max = glm::max(maxs[i], m_max);

			m_massCenter += massCenters[i];
		}
		m_massCenter /= m_particles2D.size();
	}
	template <typename T>
	void ParticlePhysics2D<T>::InitializeGrid()
	{
		CalculateMinMax();
		const auto cellSize = m_particleRadius * 2.f;
		m_particleGrid2D.Reset(cellSize, m_min, m_max);
		for (ParticleHandle i = 0; i < m_particles2D.size(); i++)
		{
			const auto& particle = m_particles2D[i];
			m_particleGrid2D.RegisterParticle(particle.m_position, i);
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::CheckCollisions()
	{
		Jobs::ParallelFor(m_particles2D.size(), [&](unsigned i) {
			const ParticleHandle particleHandle = i;
			const auto& particle = m_particles2D[particleHandle];
			const auto& coordinate = m_particleGrid2D.GetCoordinate(particle.m_position);
			for (int dx = -1; dx <= 1; dx++)
			{
				for (int dy = -1; dy <= 1; dy++)
				{
					const auto x = coordinate.x + dx;
					const auto y = coordinate.y + dy;
					if (x < 0) continue;
					if (y < 0) continue;
					if (x >= m_particleGrid2D.m_resolution.x) continue;
					if (y >= m_particleGrid2D.m_resolution.y) continue;
					const auto& cell = m_particleGrid2D.RefCell(glm::ivec2(x, y));
					for (int i = 0; i < cell.m_atomCount; i++)
					{
						if (const auto particleHandle2 = cell.m_atomHandles[i]; particleHandle != particleHandle2) {
							SolveCollision(particleHandle, particleHandle2);
						}
					}
				}
			}
			});
	}

	template <typename T>
	float ParticlePhysics2D<T>::GetDistanceToCenter(const glm::vec2& direction)
	{
		const auto threadCount = Jobs::Workers().Size();
		std::vector<float> maxDistances;
		maxDistances.resize(threadCount);
		for (int i = 0; i < threadCount; i++)
		{
			maxDistances[i] = FLT_MIN;
		}
		Jobs::ParallelFor(m_particles2D.size(), [&](const unsigned i, const unsigned threadIndex)
			{
				const auto& particle = m_particles2D[i];
				const auto distance = glm::length(glm::closestPointOnLine(particle.m_position, glm::vec2(0.0f), direction * 10000.0f));
				maxDistances[threadIndex] = glm::max(maxDistances[threadIndex], distance);
			});
		float maxDistance = FLT_MIN;
		for (int i = 0; i < threadCount; i++)
		{
			maxDistance = glm::max(maxDistances[i], maxDistance);
		}
		return maxDistance;
	}

	template <typename T>
	float ParticlePhysics2D<T>::GetTotalMoveDelta() const
	{
		return m_totalMoveDelta;
	}

	template <typename T>
	float ParticlePhysics2D<T>::GetDeltaTime() const
	{
		return m_deltaTime;
	}

	template <typename T>
	void ParticlePhysics2D<T>::Reset(const float deltaTime)
	{
		m_deltaTime = deltaTime;
		m_particles2D.clear();
		m_particleGrid2D = {};
		m_massCenter = glm::vec2(0.0f);
		m_activeness = 0.0f;
		m_particleSoftness = 0.5f;
		m_particleRadius = 0.01f;
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
	void ParticlePhysics2D<T>::SimulateByTime(float time,
		const std::function<void(Particle2D<T>& collisionParticle)>& modifyParticleFunc)
	{
		const auto count = static_cast<size_t>(glm::round(time / m_deltaTime));
		for (size_t i{ count }; i--;)
		{
			Update(modifyParticleFunc);
		}
	}

	template <typename T>
	void ParticlePhysics2D<T>::Simulate(const size_t iterations,
		const std::function<void(Particle2D<T>& particle)>& modifyParticleFunc)
	{
		for (size_t i{ iterations }; i--;)
		{
			Update(modifyParticleFunc);
		}
	}

	template <typename T>
	glm::vec2 ParticlePhysics2D<T>::GetMassCenter() const
	{
		return m_massCenter;
	}

	template <typename T>
	void ParticlePhysics2D<T>::OnInspect(const std::function<void(glm::vec2 position)>& func, const std::function<void(ImVec2 origin, float zoomFactor, ImDrawList*)>& drawFunc, bool showGrid)
	{
		static auto scrolling = glm::vec2(0.0f);
		static float zoomFactor = 100.f;
		ImGui::Text(("Total move delta: " + std::to_string(m_totalMoveDelta) +
			" | Total activeness: " + std::to_string(m_activeness) +
			" | Particle count: " + std::to_string(m_particles2D.size()) +
			" | Simulation time: " + std::to_string(m_simulationTime)).c_str());
		if (ImGui::Button("Recenter")) {
			scrolling = glm::vec2(0.0f);
		}
		ImGui::SameLine();
		ImGui::DragFloat("Zoom", &zoomFactor, zoomFactor / 100.0f, 0.1f, 1000.0f);
		zoomFactor = glm::clamp(zoomFactor, 0.01f, 1000.0f);
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
		if (isMouseHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			func(glm::vec2(mousePosInCanvas.x, mousePosInCanvas.y));
		}
		const size_t mod = m_particles2D.size() / 5000;
		int index = 0;
		for (const auto& particle : m_particles2D) {
			index++;
			if (mod > 1 && index % mod != 0) continue;
			const auto& pointPosition = particle.m_position;
			const auto& pointRadius = m_particleRadius;
			const auto& pointColor = particle.m_color;
			const auto canvasPosition = ImVec2(origin.x + pointPosition.x * zoomFactor,
				origin.y + pointPosition.y * zoomFactor);

			drawList->AddCircleFilled(canvasPosition,
				glm::clamp(zoomFactor * pointRadius, 1.0f, 100.0f),
				IM_COL32(255.0f * pointColor.x, 255.0f * pointColor.y, 255.0f * pointColor.z, 255.0f * pointColor.w));
		}
		if (showGrid) {
			for (int i = 0; i < m_particleGrid2D.m_resolution.x; i++)
			{
				for (int j = 0; j < m_particleGrid2D.m_resolution.y; j++)
				{
					ImVec2 min = ImVec2(m_min.x + i * m_particleGrid2D.m_cellSize, m_min.y + j * m_particleGrid2D.m_cellSize);
					drawList->AddQuad(
						min * zoomFactor + origin,
						ImVec2(min.x + m_particleGrid2D.m_cellSize, min.y) * zoomFactor + origin,
						ImVec2(min.x + m_particleGrid2D.m_cellSize, min.y + m_particleGrid2D.m_cellSize) * zoomFactor + origin,
						ImVec2(min.x, min.y + m_particleGrid2D.m_cellSize) * zoomFactor + origin,
						IM_COL32(0, 255, 0, 255));
				}
			}
		}
		drawFunc(origin, zoomFactor, drawList);
		drawList->PopClipRect();

	}
}
