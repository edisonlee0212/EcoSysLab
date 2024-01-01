#pragma once
#include "Skeleton.hpp"
#include "EnvironmentGrid.hpp"
#include "TreeOccupancyGrid.hpp"
using namespace EvoEngine;
namespace EcoSysLab
{
#pragma region Utilities
	enum class BudType {
		Apical,
		Lateral,
		Leaf,
		Fruit
	};

	enum class BudStatus {
		Dormant,
		Died,
		Removed
	};

	struct ReproductiveModule
	{
		float m_maturity = 0.0f;
		float m_health = 1.0f;
		glm::mat4 m_transform = glm::mat4(0.0f);
		void Reset();
	};

	class Bud {
	public:
		float m_flushingRate;
		float m_extinctionRate;

		BudType m_type = BudType::Apical;
		BudStatus m_status = BudStatus::Dormant;

		glm::quat m_localRotation = glm::vec3(0.0f);

		//-1.0 means the no fruit.
		ReproductiveModule m_reproductiveModule;
		glm::vec3 m_markerDirection = glm::vec3(0.0f);
		size_t m_markerCount = 0;
		float m_shootFlux = 0.0f;
	};

	struct ShootFlux {
		float m_value = 0.0f;
	};

	struct TreeVoxelData
	{
		NodeHandle m_nodeHandle = -1;
		NodeHandle m_flowHandle = -1;
		unsigned m_referenceCount = 0;
	};

#pragma endregion

	struct InternodeGrowthData {
		float m_internodeLength = 0.0f;

		glm::quat m_localRotation = glm::vec3(0.0f);

		bool m_isMaxChild = false;
		bool m_lateral = false;
		float m_startAge = 0;
		float m_inhibitorSink = 0;
		glm::quat m_desiredLocalRotation = glm::vec3(0.0f);
		glm::quat m_desiredGlobalRotation = glm::vec3(0.0f);
		glm::vec3 m_desiredGlobalPosition = glm::vec3(0.0f);
		float m_sagging = 0;

		int m_order = 0;
		float m_descendentTotalBiomass = 0;
		float m_biomass = 0;
		float m_extraMass = 0.0f;
		
		float m_temperature = 0.0f;
		float m_desiredGrowthPotential = 0.0f;
		float m_pipeResistance = 0.0f;
		float m_lightIntensity = 1.0f;
		float m_actualGrowthPotential = 0.0f;
		float m_vigor = 0.0f;
		float m_spaceOccupancy = 0.0f;
		glm::vec3 m_lightDirection = glm::vec3(0, 1, 0);
		
		/**
		 * List of buds, first one will always be the apical bud which points forward.
		 */
		std::vector<Bud> m_buds;
		std::vector<glm::mat4> m_leaves;
		std::vector<glm::mat4> m_fruits;
	};

	struct ShootStemGrowthData {
		int m_order = 0;
	};

	struct ShootGrowthData {
		Octree<TreeVoxelData> m_octree = {};

		size_t m_maxMarkerCount = 0;

		ShootFlux m_shootFlux = {};

		std::vector<ReproductiveModule> m_droppedLeaves;
		std::vector<ReproductiveModule> m_droppedFruits;

		glm::vec3 m_desiredMin = glm::vec3(FLT_MAX);
		glm::vec3 m_desiredMax = glm::vec3(FLT_MIN);
	};


	typedef Skeleton<ShootGrowthData, ShootStemGrowthData, InternodeGrowthData> ShootSkeleton;
}