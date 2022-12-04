#pragma once

#include "ecosyslab_export.h"
#include "SoilStructure.hpp"
using namespace UniEngine;
namespace EcoSysLab {
	class SoilModel {
	public:
		[[nodiscard]] float GetWater(const glm::vec3& position) const;
		[[nodiscard]] float GetDensity(const glm::vec3& position) const;
		[[nodiscard]] float GetNutrient(const glm::vec3& position) const;

		[[nodiscard]] float AddWater(const glm::vec3& position, float value);
		[[nodiscard]] float AddDensity(const glm::vec3& position, float value);
		[[nodiscard]] float AddNutrient(const glm::vec3& position, float value);
	};
}