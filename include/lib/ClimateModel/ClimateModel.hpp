#pragma once
#include "EnvironmentGrid.hpp"

using namespace EvoEngine;
namespace EcoSysLab {
	class ClimateParameters {
	public:

	};
	class ClimateModel {
	public:
		float m_monthAvgTemp[12] = { 38, 42, 46, 54, 61, 68, 77, 83, 77, 67, 55, 43 };
		float m_time = 0.0f;
		EnvironmentGrid m_environmentGrid{};
		[[nodiscard]] float GetTemperature(const glm::vec3& position) const;
		[[nodiscard]] float GetEnvironmentalLight(const glm::vec3& position, glm::vec3& lightDirection) const;

		void Initialize(const ClimateParameters& climateParameters);
	};
}
