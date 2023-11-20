//
// Created by lllll on 10/21/2022.
//

#include "TreeModel.hpp"

using namespace EcoSysLab;
void ReproductiveModule::Reset()
{
	m_maturity = 0.0f;
	m_health = 1.0f;
	m_transform = glm::mat4(0.0f);
}

void TreeModel::ResetReproductiveModule()
{
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		auto& internode = m_shootSkeleton.RefNode(*it);
		auto& internodeData = internode.m_data;
		auto& buds = internodeData.m_buds;
		for (auto& bud : buds)
		{
			if (bud.m_status == BudStatus::Removed) continue;
			if (bud.m_type == BudType::Fruit || bud.m_type == BudType::Leaf)
			{
				bud.m_status = BudStatus::Dormant;
				bud.m_reproductiveModule.Reset();
			}
		}
	}
	m_fruitCount = m_leafCount = 0;
}

void TreeModel::RegisterVoxel(const glm::mat4& globalTransform, ClimateModel& climateModel, const ShootGrowthController& shootGrowthParameters)
{
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	auto& environmentGrid = climateModel.m_environmentGrid;
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		const auto& internode = m_shootSkeleton.RefNode(*it);
		const auto& internodeInfo = internode.m_info;
		const float shadowSize = environmentGrid.m_settings.m_shadowIntensity;
		const float biomass = internodeInfo.m_thickness;
		const glm::vec3 worldPosition = globalTransform * glm::vec4(internodeInfo.m_globalPosition, 1.0f);
		environmentGrid.AddShadowValue(worldPosition , shadowSize);
		environmentGrid.AddBiomass(worldPosition, biomass);
		if (internode.IsEndNode()) {
			InternodeVoxelRegistration registration;
			registration.m_position = worldPosition;
			registration.m_nodeHandle = *it;
			registration.m_treeModelIndex = m_index;
			registration.m_thickness = internodeInfo.m_thickness;
			environmentGrid.AddNode(registration);
		}
	}
}

void TreeModel::CalculateInternodeTransforms()
{
	m_shootSkeleton.m_min = glm::vec3(FLT_MAX);
	m_shootSkeleton.m_max = glm::vec3(FLT_MIN);
	for (const auto& nodeHandle : m_shootSkeleton.RefSortedNodeList()) {
		auto& node = m_shootSkeleton.RefNode(nodeHandle);
		auto& nodeInfo = node.m_info;
		auto& nodeData = node.m_data;
		if (node.GetParentHandle() != -1) {
			auto& parentInfo = m_shootSkeleton.RefNode(node.GetParentHandle()).m_info;
			nodeInfo.m_globalRotation =
				parentInfo.m_globalRotation * nodeData.m_localRotation;
			nodeInfo.m_globalDirection = glm::normalize(nodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
			nodeInfo.m_globalPosition =
				parentInfo.m_globalPosition
				+ parentInfo.m_length * parentInfo.m_globalDirection;
			auto parentRegulatedUp = parentInfo.m_regulatedGlobalRotation * glm::vec3(0, 1, 0);
			auto regulatedUp = glm::normalize(glm::cross(glm::cross(nodeInfo.m_globalDirection, parentRegulatedUp), nodeInfo.m_globalDirection));
			nodeInfo.m_regulatedGlobalRotation = glm::quatLookAt(nodeInfo.m_globalDirection, regulatedUp);
		}
		m_shootSkeleton.m_min = glm::min(m_shootSkeleton.m_min, nodeInfo.m_globalPosition);
		m_shootSkeleton.m_max = glm::max(m_shootSkeleton.m_max, nodeInfo.m_globalPosition);
		const auto endPosition = nodeInfo.m_globalPosition
			+ nodeInfo.m_length * nodeInfo.m_globalDirection;
		m_shootSkeleton.m_min = glm::min(m_shootSkeleton.m_min, endPosition);
		m_shootSkeleton.m_max = glm::max(m_shootSkeleton.m_max, endPosition);
	}
}

void TreeModel::CalculateRootNodeTransforms()
{
	m_rootSkeleton.m_min = glm::vec3(FLT_MAX);
	m_rootSkeleton.m_max = glm::vec3(FLT_MIN);
	for (const auto& nodeHandle : m_rootSkeleton.RefSortedNodeList()) {
		auto& node = m_rootSkeleton.RefNode(nodeHandle);
		auto& nodeInfo = node.m_info;
		auto& nodeData = node.m_data;
		if (node.GetParentHandle() != -1) {
			auto& parentInfo = m_rootSkeleton.RefNode(node.GetParentHandle()).m_info;
			nodeInfo.m_globalRotation =
				parentInfo.m_globalRotation * nodeData.m_localRotation;
			nodeInfo.m_globalDirection = glm::normalize(nodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
			nodeInfo.m_globalPosition =
				parentInfo.m_globalPosition
				+ parentInfo.m_length * parentInfo.m_globalDirection
				+ nodeData.m_localPosition;
			auto parentRegulatedUp = parentInfo.m_regulatedGlobalRotation * glm::vec3(0, 1, 0);
			auto regulatedUp = glm::normalize(glm::cross(glm::cross(nodeInfo.m_globalDirection, parentRegulatedUp), nodeInfo.m_globalDirection));
			nodeInfo.m_regulatedGlobalRotation = glm::quatLookAt(nodeInfo.m_globalDirection, regulatedUp);
		}
		m_rootSkeleton.m_min = glm::min(m_rootSkeleton.m_min, nodeInfo.m_globalPosition);
		m_rootSkeleton.m_max = glm::max(m_rootSkeleton.m_max, nodeInfo.m_globalPosition);
		const auto endPosition = nodeInfo.m_globalPosition
			+ nodeInfo.m_length * nodeInfo.m_globalDirection;
		m_rootSkeleton.m_min = glm::min(m_rootSkeleton.m_min, endPosition);
		m_rootSkeleton.m_max = glm::max(m_rootSkeleton.m_max, endPosition);
	}
}

void TreeModel::PruneInternode(NodeHandle internodeHandle)
{
	m_shootSkeleton.RecycleNode(internodeHandle,
		[&](FlowHandle flowHandle) {},
		[&](NodeHandle nodeHandle)
		{});
}

void TreeModel::PruneRootNode(NodeHandle rootNodeHandle)
{
	m_rootSkeleton.RecycleNode(rootNodeHandle,
		[&](FlowHandle flowHandle) {},
		[&](NodeHandle nodeHandle)
		{});
}

void TreeModel::HarvestFruits(const std::function<bool(const ReproductiveModule& fruit)>& harvestFunction)
{
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	m_fruitCount = 0;

	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		auto& internode = m_shootSkeleton.RefNode(*it);
		auto& internodeData = internode.m_data;
		auto& buds = internodeData.m_buds;
		for (auto& bud : buds)
		{
			if (bud.m_type != BudType::Fruit || bud.m_status != BudStatus::Flushed) continue;

			if (harvestFunction(bud.m_reproductiveModule)) {
				bud.m_reproductiveModule.Reset();
				bud.m_status = BudStatus::Died;
			}
			else if (bud.m_reproductiveModule.m_maturity > 0) m_fruitCount++;

		}
	}
}

void TreeModel::ApplyTropism(const glm::vec3& targetDir, float tropism, glm::vec3& front, glm::vec3& up) {
	const glm::vec3 dir = glm::normalize(targetDir);
	const float dotP = glm::abs(glm::dot(front, dir));
	if (dotP < 0.99f && dotP > -0.99f) {
		const glm::vec3 left = glm::cross(front, dir);
		const float maxAngle = glm::acos(dotP);
		const float rotateAngle = maxAngle * tropism;
		front = glm::normalize(
			glm::rotate(front, glm::min(maxAngle, rotateAngle), left));
		up = glm::normalize(glm::cross(glm::cross(front, up), front));
	}
}

void TreeModel::ApplyTropism(const glm::vec3& targetDir, float tropism, glm::quat& rotation) {
	auto front = rotation * glm::vec3(0, 0, -1);
	auto up = rotation * glm::vec3(0, 1, 0);
	ApplyTropism(targetDir, tropism, front, up);
	rotation = glm::quatLookAt(front, up);
}

bool TreeModel::Grow(float deltaTime, const glm::mat4& globalTransform, VoxelSoilModel& soilModel, ClimateModel& climateModel,
	const RootGrowthController& rootGrowthParameters,
	const ShootGrowthController& shootGrowthParameters)
{
	m_currentDeltaTime = deltaTime;
	bool treeStructureChanged = false;
	bool rootStructureChanged = false;
	if (!m_initialized) {
		Initialize(shootGrowthParameters, rootGrowthParameters);
		treeStructureChanged = true;
		rootStructureChanged = true;
	}
	//Collect water from roots.
	if (m_treeGrowthSettings.m_enableRoot) {
		m_rootSkeleton.SortLists();
		const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
		CollectRootFlux(globalTransform, soilModel, sortedRootNodeList, rootGrowthParameters);
		RootGrowthRequirement newRootGrowthRequirement;
		CalculateVigorRequirement(rootGrowthParameters, newRootGrowthRequirement);
		m_rootSkeleton.m_data.m_vigorRequirement = newRootGrowthRequirement;
	}
	//Collect light from branches.
	if (m_treeGrowthSettings.m_enableShoot) {
		m_shootSkeleton.SortLists();
		const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
		CollectShootFlux(globalTransform, climateModel, sortedInternodeList, shootGrowthParameters);
		ShootGrowthRequirement newShootGrowthRequirement;
		CalculateVigorRequirement(shootGrowthParameters, newShootGrowthRequirement);
		m_shootSkeleton.m_data.m_vigorRequirement = newShootGrowthRequirement;
	}

	//Perform photosynthesis.
	PlantVigorAllocation();
	//Grow roots and set up nutrient requirements for next iteration.

	if (m_treeGrowthSettings.m_enableRoot && m_currentDeltaTime != 0.0f
		&& GrowRoots(globalTransform, soilModel, rootGrowthParameters)) {
		rootStructureChanged = true;
	}
	//Grow branches and set up nutrient requirements for next iteration.
	if (m_treeGrowthSettings.m_enableShoot && m_currentDeltaTime != 0.0f
		&& GrowShoots(globalTransform, climateModel, shootGrowthParameters)) {
		treeStructureChanged = true;
	}
	const int year = climateModel.m_time;
	if (year != m_ageInYear)
	{
		ResetReproductiveModule();
		m_ageInYear = year;
	}

	m_iteration++;
	m_age += m_currentDeltaTime;
	return treeStructureChanged || rootStructureChanged;
}

bool TreeModel::GrowSubTree(const float deltaTime, const NodeHandle baseInternodeHandle, const glm::mat4& globalTransform, ClimateModel& climateModel,
	const ShootGrowthController& shootGrowthParameters)
{
	m_currentDeltaTime = deltaTime;
	bool treeStructureChanged = false;
	m_shootSkeleton.SortLists();
	bool anyBranchGrown = false;
	const auto sortedInternodeList = m_shootSkeleton.GetSubTree(baseInternodeHandle);

	CollectShootFlux(globalTransform, climateModel, sortedInternodeList, shootGrowthParameters);
	ShootGrowthRequirement subTreeGrowthRequirement{};
	CalculateVigorRequirement(shootGrowthParameters, subTreeGrowthRequirement);
	if (!m_treeGrowthSettings.m_useSpaceColonization) {
		AggregateInternodeVigorRequirement(shootGrowthParameters, baseInternodeHandle);
		float vigor = 0.0f;
		vigor += subTreeGrowthRequirement.m_leafMaintenanceVigor;
		vigor += subTreeGrowthRequirement.m_leafDevelopmentalVigor;
		vigor += subTreeGrowthRequirement.m_fruitMaintenanceVigor;
		vigor += subTreeGrowthRequirement.m_fruitDevelopmentalVigor;
		vigor += subTreeGrowthRequirement.m_nodeDevelopmentalVigor;
		AllocateShootVigor(vigor, baseInternodeHandle, sortedInternodeList, subTreeGrowthRequirement, shootGrowthParameters);
	}
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		const bool graphChanged = GrowInternode(climateModel, *it, shootGrowthParameters, false);
		anyBranchGrown = anyBranchGrown || graphChanged;
	}
	if (anyBranchGrown) m_shootSkeleton.SortLists();
	treeStructureChanged = treeStructureChanged || anyBranchGrown;
	ShootGrowthPostProcess(globalTransform, climateModel, shootGrowthParameters);
	m_iteration++;
	return treeStructureChanged;
}

void TreeModel::Initialize(const ShootGrowthController& shootGrowthParameters, const RootGrowthController& rootGrowthParameters) {
	if (m_initialized) Clear();
	{
		auto& firstInternode = m_shootSkeleton.RefNode(0);
		firstInternode.m_info.m_thickness = shootGrowthParameters.m_endNodeThickness;
		firstInternode.m_data.m_internodeLength = 0.0f;
		firstInternode.m_data.m_buds.emplace_back();
		auto& apicalBud = firstInternode.m_data.m_buds.back();
		apicalBud.m_type = BudType::Apical;
		apicalBud.m_status = BudStatus::Dormant;
		apicalBud.m_vigorSink.AddVigor(shootGrowthParameters.m_internodeVigorRequirement);
		apicalBud.m_localRotation = glm::vec3(0.0f);
	}
	{
		auto& firstRootNode = m_rootSkeleton.RefNode(0);
		firstRootNode.m_info.m_thickness = 0.003f;
		firstRootNode.m_info.m_length = 0.0f;
		firstRootNode.m_data.m_localRotation = glm::vec3(0.0f);
		firstRootNode.m_data.m_verticalTropism = rootGrowthParameters.m_tropismIntensity;
		firstRootNode.m_data.m_horizontalTropism = 0;
		firstRootNode.m_data.m_vigorSink.AddVigor(rootGrowthParameters.m_rootNodeVigorRequirement);
	}
	if (m_treeGrowthSettings.m_useSpaceColonization && m_treeGrowthSettings.m_spaceColonizationAutoResize) {
		const auto gridRadius = m_treeGrowthSettings.m_spaceColonizationDetectionDistanceFactor * shootGrowthParameters.m_internodeLength;
		m_treeOccupancyGrid.Initialize(glm::vec3(-gridRadius, 0.0f, -gridRadius), glm::vec3(gridRadius), shootGrowthParameters.m_internodeLength,
			m_treeGrowthSettings.m_spaceColonizationRemovalDistanceFactor, m_treeGrowthSettings.m_spaceColonizationTheta, m_treeGrowthSettings.m_spaceColonizationDetectionDistanceFactor);
	}
	m_initialized = true;
}

void TreeModel::CollectRootFlux(const glm::mat4& globalTransform, VoxelSoilModel& soilModel, const std::vector<NodeHandle>& sortedSubTreeRootNodeList, const RootGrowthController& rootGrowthParameters)
{
	m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential = 0.0f;
	for (const auto& rootNodeHandle : sortedSubTreeRootNodeList) {
		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& rootNodeInfo = rootNode.m_info;
		auto& rootNodeData = rootNode.m_data;
		rootNodeData.m_growthPotential = 0.0f;
		auto worldSpacePosition = globalTransform * glm::translate(rootNodeInfo.m_globalPosition)[3];
		if (m_treeGrowthSettings.m_collectRootFlux) {
			if (m_treeGrowthSettings.m_useSpaceColonization) {
				rootNodeData.m_growthPotential = soilModel.IntegrateWater(worldSpacePosition, 0.2f);
				m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential += rootNodeData.m_growthPotential;
			}
			else {
				rootNodeData.m_growthPotential = soilModel.IntegrateWater(worldSpacePosition, 0.2f);
				m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential += rootNodeData.m_growthPotential;
			}
		}
		else
		{
			rootNodeData.m_growthPotential = 1.0f;
			m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential += rootNodeData.m_growthPotential;
		}
	}

}

void TreeModel::CollectShootFlux(const glm::mat4& globalTransform, ClimateModel& climateModel, const std::vector<NodeHandle>& sortedSubTreeInternodeList,
	const ShootGrowthController& shootGrowthParameters)
{
	auto& shootData = m_shootSkeleton.m_data;
	shootData.m_shootFlux.m_totalGrowthPotential = 0.0f;
	shootData.m_maxMarkerCount = 0;
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	if (m_treeGrowthSettings.m_useSpaceColonization)
	{
		if (m_treeGrowthSettings.m_spaceColonizationAutoResize) {
			auto minBound = m_shootSkeleton.m_data.m_desiredMin;
			auto maxBound = m_shootSkeleton.m_data.m_desiredMax;
			const auto originalMin = m_treeOccupancyGrid.GetMin();
			const auto originalMax = m_treeOccupancyGrid.GetMax();
			const float detectionRange = m_treeGrowthSettings.m_spaceColonizationDetectionDistanceFactor * shootGrowthParameters.m_internodeLength;
			if (minBound.x - detectionRange < originalMin.x || minBound.y < originalMin.y || minBound.z - detectionRange < originalMin.z
				|| maxBound.x + detectionRange > originalMax.x || maxBound.y + detectionRange > originalMax.y || maxBound.z + detectionRange > originalMax.z) {
				minBound -= glm::vec3(1.0f);
				maxBound += glm::vec3(1.0f);
				m_treeOccupancyGrid.Resize(minBound, maxBound);
			}
		}
		auto& voxelGrid = m_treeOccupancyGrid.RefGrid();
		for (const auto& internodeHandle : sortedInternodeList)
		{
			auto& internode = m_shootSkeleton.RefNode(internodeHandle);
			auto& internodeData = internode.m_data;
			for (auto& bud : internodeData.m_buds)
			{
				bud.m_markerDirection = glm::vec3(0.0f);
				bud.m_markerCount = 0;
			}
			internodeData.m_lightDirection = glm::vec3(0.0f);
			const auto dotMin = glm::cos(glm::radians(m_treeOccupancyGrid.GetTheta()));
			voxelGrid.ForEach(internodeData.m_desiredGlobalPosition, m_treeGrowthSettings.m_spaceColonizationRemovalDistanceFactor * shootGrowthParameters.m_internodeLength,
				[&](TreeOccupancyGridVoxelData& voxelData)
				{
					for (auto& marker : voxelData.m_markers)
					{
						const auto diff = marker.m_position - internodeData.m_desiredGlobalPosition;
						const auto distance = glm::length(diff);
						const auto direction = glm::normalize(diff);
						if (distance < m_treeGrowthSettings.m_spaceColonizationDetectionDistanceFactor * shootGrowthParameters.m_internodeLength)
						{
							if (marker.m_nodeHandle != -1) continue;
							if (distance < m_treeGrowthSettings.m_spaceColonizationRemovalDistanceFactor * shootGrowthParameters.m_internodeLength)
							{
								marker.m_nodeHandle = internodeHandle;
							}
							else
							{
								for (auto& bud : internodeData.m_buds) {
									auto budDirection = glm::normalize(internode.m_info.m_globalRotation * bud.m_localRotation * glm::vec3(0, 0, -1));
									if (glm::dot(direction, budDirection) > dotMin)
									{
										bud.m_markerDirection += direction;
										bud.m_markerCount++;
									}
								}
							}
						}

					}
				}
			);
		}
	}
	for (const auto& internodeHandle : sortedSubTreeInternodeList) {
		auto& internode = m_shootSkeleton.RefNode(internodeHandle);
		auto& internodeData = internode.m_data;
		auto& internodeInfo = internode.m_info;
		internodeData.m_growthPotential = 0.0f;
		if (m_treeGrowthSettings.m_useSpaceColonization) {
			for (const auto& bud : internodeData.m_buds)
			{
				shootData.m_maxMarkerCount = glm::max(shootData.m_maxMarkerCount, bud.m_markerCount);
			}
		}
		const glm::vec3 position = globalTransform * glm::vec4(internodeInfo.m_globalPosition, 1.0f);
		internodeData.m_growthPotential = climateModel.m_environmentGrid.IlluminationEstimation(position, internodeData.m_lightDirection);
		m_shootSkeleton.m_data.m_shootFlux.m_totalGrowthPotential += internodeData.m_growthPotential;
		if (internodeData.m_growthPotential <= glm::epsilon<float>())
		{
			internodeData.m_lightDirection = glm::normalize(internodeInfo.m_globalDirection);
		}
		internodeData.m_spaceOccupancy = climateModel.m_environmentGrid.m_voxel.Peek(position).m_totalBiomass;
	}
}

void TreeModel::PlantVigorAllocation()
{
	if (!m_treeGrowthSettings.m_collectRootFlux) {
		m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential =
			m_shootSkeleton.m_data.m_vigorRequirement.m_leafMaintenanceVigor
			+ m_shootSkeleton.m_data.m_vigorRequirement.m_leafDevelopmentalVigor
			+ m_shootSkeleton.m_data.m_vigorRequirement.m_fruitMaintenanceVigor
			+ m_shootSkeleton.m_data.m_vigorRequirement.m_fruitDevelopmentalVigor
			+ m_shootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor
			+ m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor;
	}

	float totalVigor = glm::max(m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential, m_shootSkeleton.m_data.m_shootFlux.m_totalGrowthPotential);
	if (m_treeGrowthSettings.m_enableShoot && m_treeGrowthSettings.m_enableRoot)
	{
		totalVigor = glm::min(m_rootSkeleton.m_data.m_rootFlux.m_totalGrowthPotential, m_shootSkeleton.m_data.m_shootFlux.m_totalGrowthPotential);
	}
	const float totalLeafMaintenanceVigorRequirement = m_shootSkeleton.m_data.m_vigorRequirement.m_leafMaintenanceVigor;
	const float totalLeafDevelopmentVigorRequirement = m_shootSkeleton.m_data.m_vigorRequirement.m_leafDevelopmentalVigor;
	const float totalFruitMaintenanceVigorRequirement = m_shootSkeleton.m_data.m_vigorRequirement.m_fruitMaintenanceVigor;
	const float totalFruitDevelopmentVigorRequirement = m_shootSkeleton.m_data.m_vigorRequirement.m_fruitDevelopmentalVigor;
	const float totalNodeDevelopmentalVigorRequirement = m_shootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor + m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor;

	const float leafMaintenanceVigor = glm::min(totalVigor, totalLeafMaintenanceVigorRequirement);
	const float leafDevelopmentVigor = glm::min(totalVigor - totalLeafMaintenanceVigorRequirement, totalLeafDevelopmentVigorRequirement);
	const float fruitMaintenanceVigor = glm::min(totalVigor - totalLeafMaintenanceVigorRequirement - leafDevelopmentVigor, totalFruitMaintenanceVigorRequirement);
	const float fruitDevelopmentVigor = glm::min(totalVigor - totalLeafMaintenanceVigorRequirement - leafDevelopmentVigor - fruitMaintenanceVigor, totalFruitDevelopmentVigorRequirement);
	const float nodeDevelopmentVigor = glm::min(totalVigor - totalLeafMaintenanceVigorRequirement - leafDevelopmentVigor - fruitMaintenanceVigor - fruitDevelopmentVigor, totalNodeDevelopmentalVigorRequirement);
	m_rootSkeleton.m_data.m_vigor = m_shootSkeleton.m_data.m_vigor = 0.0f;
	if (totalLeafMaintenanceVigorRequirement != 0.0f) {
		m_shootSkeleton.m_data.m_vigor += leafMaintenanceVigor * m_shootSkeleton.m_data.m_vigorRequirement.m_leafMaintenanceVigor
			/ totalLeafMaintenanceVigorRequirement;
	}
	if (totalLeafDevelopmentVigorRequirement != 0.0f) {
		m_shootSkeleton.m_data.m_vigor += leafDevelopmentVigor * m_shootSkeleton.m_data.m_vigorRequirement.m_leafDevelopmentalVigor
			/ totalLeafDevelopmentVigorRequirement;
	}
	if (totalFruitMaintenanceVigorRequirement != 0.0f) {
		m_shootSkeleton.m_data.m_vigor += fruitMaintenanceVigor * m_shootSkeleton.m_data.m_vigorRequirement.m_fruitMaintenanceVigor
			/ totalFruitMaintenanceVigorRequirement;
	}
	if (totalFruitDevelopmentVigorRequirement != 0.0f) {
		m_shootSkeleton.m_data.m_vigor += fruitDevelopmentVigor * m_shootSkeleton.m_data.m_vigorRequirement.m_fruitDevelopmentalVigor
			/ totalFruitDevelopmentVigorRequirement;
	}


	if (m_treeGrowthSettings.m_autoBalance && m_treeGrowthSettings.m_enableRoot && m_treeGrowthSettings.m_enableShoot) {
		m_vigorRatio.m_shootVigorWeight = m_rootSkeleton.RefSortedNodeList().size() * m_shootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor;
		m_vigorRatio.m_rootVigorWeight = m_shootSkeleton.RefSortedNodeList().size() * m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor;
	}
	else {
		m_vigorRatio.m_rootVigorWeight = m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor / totalNodeDevelopmentalVigorRequirement;
		m_vigorRatio.m_shootVigorWeight = m_shootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor / totalNodeDevelopmentalVigorRequirement;
	}

	if (m_vigorRatio.m_shootVigorWeight + m_vigorRatio.m_rootVigorWeight != 0.0f) {
		m_rootSkeleton.m_data.m_vigor += nodeDevelopmentVigor * m_vigorRatio.m_rootVigorWeight / (m_vigorRatio.m_shootVigorWeight + m_vigorRatio.m_rootVigorWeight);
		m_shootSkeleton.m_data.m_vigor += nodeDevelopmentVigor * m_vigorRatio.m_shootVigorWeight / (m_vigorRatio.m_shootVigorWeight + m_vigorRatio.m_rootVigorWeight);
	}
}

bool TreeModel::GrowRoots(const glm::mat4& globalTransform, VoxelSoilModel& soilModel, const RootGrowthController& rootGrowthParameters)
{
	bool rootStructureChanged = false;
	m_rootSkeleton.SortLists();
	const bool anyRootPruned = PruneRootNodes(rootGrowthParameters);
	if (anyRootPruned) m_rootSkeleton.SortLists();
	rootStructureChanged = rootStructureChanged || anyRootPruned;

	bool anyRootGrown = false;
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
	AggregateRootVigorRequirement(rootGrowthParameters, 0);
	AllocateRootVigor(rootGrowthParameters);
	for (auto it = sortedRootNodeList.rbegin(); it != sortedRootNodeList.rend(); it++) {
		const bool graphChanged = GrowRootNode(soilModel, *it, rootGrowthParameters);
		anyRootGrown = anyRootGrown || graphChanged;
	}
	if (anyRootGrown) m_rootSkeleton.SortLists();
	rootStructureChanged = rootStructureChanged || anyRootGrown;

	RootGrowthPostProcess(globalTransform, soilModel, rootGrowthParameters);
	return rootStructureChanged;
}

void TreeModel::RootGrowthPostProcess(const glm::mat4& globalTransform, VoxelSoilModel& soilModel, const RootGrowthController& rootGrowthParameters)
{
	{
		m_rootSkeleton.m_min = glm::vec3(FLT_MAX);
		m_rootSkeleton.m_max = glm::vec3(FLT_MIN);
		const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
		for (auto it = sortedRootNodeList.rbegin(); it != sortedRootNodeList.rend(); ++it) {
			CalculateThickness(*it, rootGrowthParameters);
		}
		for (const auto& rootNodeHandle : sortedRootNodeList) {
			auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
			auto& rootNodeData = rootNode.m_data;
			auto& rootNodeInfo = rootNode.m_info;

			if (rootNode.GetParentHandle() == -1) {
				rootNodeInfo.m_globalPosition = glm::vec3(0.0f);
				rootNodeData.m_localRotation = glm::vec3(0.0f);
				rootNodeInfo.m_globalRotation = rootNodeInfo.m_regulatedGlobalRotation = glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f);
				rootNodeInfo.m_globalDirection = glm::normalize(rootNodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
				rootNodeData.m_rootDistance = rootNodeInfo.m_length / rootGrowthParameters.m_rootNodeLength;
			}
			else {
				auto& parentRootNode = m_rootSkeleton.RefNode(rootNode.GetParentHandle());
				rootNodeData.m_rootDistance = parentRootNode.m_data.m_rootDistance + rootNodeInfo.m_length / rootGrowthParameters.m_rootNodeLength;
				rootNodeInfo.m_globalRotation =
					parentRootNode.m_info.m_globalRotation * rootNodeData.m_localRotation;
				rootNodeInfo.m_globalDirection = glm::normalize(rootNodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
				rootNodeInfo.m_globalPosition =
					parentRootNode.m_info.m_globalPosition
					+ parentRootNode.m_info.m_length * parentRootNode.m_info.m_globalDirection;


				auto parentRegulatedUp = parentRootNode.m_info.m_regulatedGlobalRotation * glm::vec3(0, 1, 0);
				auto regulatedUp = glm::normalize(glm::cross(glm::cross(rootNodeInfo.m_globalDirection, parentRegulatedUp), rootNodeInfo.m_globalDirection));
				rootNodeInfo.m_regulatedGlobalRotation = glm::quatLookAt(rootNodeInfo.m_globalDirection, regulatedUp);

			}
			m_rootSkeleton.m_min = glm::min(m_rootSkeleton.m_min, rootNodeInfo.m_globalPosition);
			m_rootSkeleton.m_max = glm::max(m_rootSkeleton.m_max, rootNodeInfo.m_globalPosition);
			const auto endPosition = rootNodeInfo.m_globalPosition
				+ rootNodeInfo.m_length * rootNodeInfo.m_globalDirection;
			m_rootSkeleton.m_min = glm::min(m_rootSkeleton.m_min, endPosition);
			m_rootSkeleton.m_max = glm::max(m_rootSkeleton.m_max, endPosition);
		}
	};
	SampleSoilDensity(globalTransform, soilModel);
	SampleNitrite(globalTransform, soilModel);

	if (m_treeGrowthSettings.m_enableRootCollisionDetection)
	{
		const float minRadius = rootGrowthParameters.m_endNodeThickness * 4.0f;
		CollisionDetection(minRadius, m_rootSkeleton.m_data.m_octree, m_rootSkeleton);
	}
	m_rootNodeOrderCounts.clear();
	{
		int maxOrder = 0;
		const auto& sortedFlowList = m_rootSkeleton.RefSortedFlowList();
		for (const auto& flowHandle : sortedFlowList) {
			auto& flow = m_rootSkeleton.RefFlow(flowHandle);
			auto& flowData = flow.m_data;
			if (flow.GetParentHandle() == -1) {
				flowData.m_order = 0;
			}
			else {
				auto& parentFlow = m_rootSkeleton.RefFlow(flow.GetParentHandle());
				if (flow.IsApical()) flowData.m_order = parentFlow.m_data.m_order;
				else flowData.m_order = parentFlow.m_data.m_order + 1;
			}
			maxOrder = glm::max(maxOrder, flowData.m_order);
		}
		m_rootNodeOrderCounts.resize(maxOrder + 1);
		std::fill(m_rootNodeOrderCounts.begin(), m_rootNodeOrderCounts.end(), 0);
		const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
		for (const auto& rootNodeHandle : sortedRootNodeList)
		{
			auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
			const auto order = m_rootSkeleton.RefFlow(rootNode.GetFlowHandle()).m_data.m_order;
			rootNode.m_data.m_order = order;
			m_rootNodeOrderCounts[order]++;
		}
		m_rootSkeleton.CalculateFlows();
	};
}

bool TreeModel::GrowShoots(const glm::mat4& globalTransform, ClimateModel& climateModel, const ShootGrowthController& shootGrowthParameters) {
	bool treeStructureChanged = false;

	const bool anyBranchPruned = PruneInternodes(globalTransform, climateModel, shootGrowthParameters);
	if (anyBranchPruned) m_shootSkeleton.SortLists();
	treeStructureChanged = treeStructureChanged || anyBranchPruned;
	bool anyBranchGrown = false;
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	if (!m_treeGrowthSettings.m_useSpaceColonization) {
		AggregateInternodeVigorRequirement(shootGrowthParameters, 0);
		AllocateShootVigor(m_shootSkeleton.m_data.m_vigor, 0, sortedInternodeList, m_shootSkeleton.m_data.m_vigorRequirement, shootGrowthParameters);
	}
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		const bool graphChanged = GrowInternode(climateModel, *it, shootGrowthParameters, true);
		anyBranchGrown = anyBranchGrown || graphChanged;
	}
	if (anyBranchGrown) m_shootSkeleton.SortLists();
	treeStructureChanged = treeStructureChanged || anyBranchGrown;
	ShootGrowthPostProcess(globalTransform, climateModel, shootGrowthParameters);
	return treeStructureChanged;
}



void TreeModel::ShootGrowthPostProcess(const glm::mat4& globalTransform, ClimateModel& climateModel, const ShootGrowthController& shootGrowthParameters)
{
	{
		m_shootSkeleton.m_min = glm::vec3(FLT_MAX);
		m_shootSkeleton.m_max = glm::vec3(FLT_MIN);
		m_shootSkeleton.m_data.m_desiredMin = glm::vec3(FLT_MAX);
		m_shootSkeleton.m_data.m_desiredMax = glm::vec3(FLT_MIN);

		const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
		for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
			auto internodeHandle = *it;
			CalculateThicknessAndSagging(internodeHandle, shootGrowthParameters);
		}
		for (const auto& internodeHandle : sortedInternodeList) {
			auto& internode = m_shootSkeleton.RefNode(internodeHandle);
			auto& internodeData = internode.m_data;
			auto& internodeInfo = internode.m_info;

			internodeInfo.m_length = internodeData.m_internodeLength * glm::pow(internodeInfo.m_thickness / shootGrowthParameters.m_endNodeThickness, shootGrowthParameters.m_internodeLengthThicknessFactor);

			if (internode.GetParentHandle() == -1) {
				internodeInfo.m_globalPosition = internodeData.m_desiredGlobalPosition = glm::vec3(0.0f);
				internodeData.m_localRotation = glm::vec3(0.0f);
				internodeInfo.m_globalRotation = internodeInfo.m_regulatedGlobalRotation = internodeData.m_desiredGlobalRotation = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f);
				internodeInfo.m_globalDirection = glm::normalize(internodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
				internodeInfo.m_rootDistance = internodeInfo.m_length;
			}
			else {
				auto& parentInternode = m_shootSkeleton.RefNode(internode.GetParentHandle());
				internodeInfo.m_rootDistance = parentInternode.m_info.m_rootDistance + internodeInfo.m_length;
				internodeInfo.m_globalRotation =
					parentInternode.m_info.m_globalRotation * internodeData.m_localRotation;

#pragma region Apply Sagging
				const auto& parentNode = m_shootSkeleton.RefNode(
					internode.GetParentHandle());
				auto parentGlobalRotation = parentNode.m_info.m_globalRotation;
				internodeInfo.m_globalRotation = parentGlobalRotation * internodeData.m_desiredLocalRotation;
				auto front = internodeInfo.m_globalRotation * glm::vec3(0, 0, -1);
				auto up = internodeInfo.m_globalRotation * glm::vec3(0, 1, 0);
				float dotP = glm::abs(glm::dot(front, m_currentGravityDirection));
				ApplyTropism(m_currentGravityDirection, internodeData.m_sagging * (1.0f - dotP), front, up);
				internodeInfo.m_globalRotation = glm::quatLookAt(front, up);
				internodeData.m_localRotation = glm::inverse(parentGlobalRotation) * internodeInfo.m_globalRotation;

				auto parentRegulatedUp = parentNode.m_info.m_regulatedGlobalRotation * glm::vec3(0, 1, 0);
				auto regulatedUp = glm::normalize(glm::cross(glm::cross(front, parentRegulatedUp), front));
				internodeInfo.m_regulatedGlobalRotation = glm::quatLookAt(front, regulatedUp);
#pragma endregion
				internodeInfo.m_globalDirection = glm::normalize(internodeInfo.m_globalRotation * glm::vec3(0, 0, -1));
				internodeInfo.m_globalPosition =
					parentInternode.m_info.m_globalPosition
					+ parentInternode.m_info.m_length * parentInternode.m_info.m_globalDirection;

				internodeData.m_desiredGlobalRotation = parentNode.m_data.m_desiredGlobalRotation * internodeData.m_desiredLocalRotation;
				auto parentDesiredFront = parentNode.m_data.m_desiredGlobalRotation * glm::vec3(0, 0, -1);
				internodeData.m_desiredGlobalPosition = parentNode.m_data.m_desiredGlobalPosition +
					parentInternode.m_info.m_length * parentDesiredFront;
			}

			m_shootSkeleton.m_min = glm::min(m_shootSkeleton.m_min, internodeInfo.m_globalPosition);
			m_shootSkeleton.m_max = glm::max(m_shootSkeleton.m_max, internodeInfo.m_globalPosition);
			const auto endPosition = internodeInfo.m_globalPosition
				+ internodeInfo.m_length * internodeInfo.m_globalDirection;
			m_shootSkeleton.m_min = glm::min(m_shootSkeleton.m_min, endPosition);
			m_shootSkeleton.m_max = glm::max(m_shootSkeleton.m_max, endPosition);

			m_shootSkeleton.m_data.m_desiredMin = glm::min(m_shootSkeleton.m_data.m_desiredMin, internodeData.m_desiredGlobalPosition);
			m_shootSkeleton.m_data.m_desiredMax = glm::max(m_shootSkeleton.m_data.m_desiredMax, internodeData.m_desiredGlobalPosition);
			const auto desiredGlobalDirection = internodeData.m_desiredGlobalRotation * glm::vec3(0, 0, -1);
			const auto desiredEndPosition = internodeData.m_desiredGlobalPosition
				+ internodeInfo.m_length * desiredGlobalDirection;
			m_shootSkeleton.m_data.m_desiredMin = glm::min(m_shootSkeleton.m_data.m_desiredMin, desiredEndPosition);
			m_shootSkeleton.m_data.m_desiredMax = glm::max(m_shootSkeleton.m_data.m_desiredMax, desiredEndPosition);
		}
		SampleTemperature(globalTransform, climateModel);
	};

	if (m_treeGrowthSettings.m_enableBranchCollisionDetection)
	{
		const float minRadius = shootGrowthParameters.m_endNodeThickness * 4.0f;
		CollisionDetection(minRadius, m_shootSkeleton.m_data.m_octree, m_shootSkeleton);
	}
	m_internodeOrderCounts.clear();
	m_fruitCount = m_leafCount = 0;
	{
		int maxOrder = 0;
		const auto& sortedFlowList = m_shootSkeleton.RefSortedFlowList();
		for (const auto& flowHandle : sortedFlowList) {
			auto& flow = m_shootSkeleton.RefFlow(flowHandle);
			auto& flowData = flow.m_data;
			if (flow.GetParentHandle() == -1) {
				flowData.m_order = 0;
			}
			else {
				auto& parentFlow = m_shootSkeleton.RefFlow(flow.GetParentHandle());
				if (flow.IsApical()) flowData.m_order = parentFlow.m_data.m_order;
				else flowData.m_order = parentFlow.m_data.m_order + 1;
			}
			maxOrder = glm::max(maxOrder, flowData.m_order);
		}
		m_internodeOrderCounts.resize(maxOrder + 1);
		std::fill(m_internodeOrderCounts.begin(), m_internodeOrderCounts.end(), 0);
		const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
		for (const auto& internodeHandle : sortedInternodeList)
		{
			auto& internode = m_shootSkeleton.RefNode(internodeHandle);
			const auto order = m_shootSkeleton.RefFlow(internode.GetFlowHandle()).m_data.m_order;
			internode.m_data.m_order = order;
			m_internodeOrderCounts[order]++;

			for (const auto& bud : internode.m_data.m_buds)
			{
				if (bud.m_status != BudStatus::Flushed || bud.m_reproductiveModule.m_maturity <= 0) continue;
				if (bud.m_type == BudType::Fruit)
				{
					m_fruitCount++;
				}
				else if (bud.m_type == BudType::Leaf)
				{
					m_leafCount++;
				}
			}

		}
		m_shootSkeleton.CalculateFlows();
	}
}


bool TreeModel::ElongateRoot(VoxelSoilModel& soilModel, const float extendLength, NodeHandle rootNodeHandle, const RootGrowthController& rootGrowthParameters,
	float& collectedAuxin) {
	bool graphChanged = false;
	auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
	const auto rootNodeLength = rootGrowthParameters.m_rootNodeLength;
	auto& rootNodeData = rootNode.m_data;
	auto& rootNodeInfo = rootNode.m_info;
	rootNodeInfo.m_length += extendLength;
	float extraLength = rootNodeInfo.m_length - rootNodeLength;
	//If we need to add a new end node
	if (extraLength > 0) {
		graphChanged = true;
		rootNodeInfo.m_length = rootNodeLength;
		const auto desiredGlobalRotation = rootNodeInfo.m_globalRotation * glm::quat(glm::vec3(
			glm::radians(rootGrowthParameters.m_apicalAngle(rootNode)), 0.0f,
			glm::radians(rootGrowthParameters.m_rollAngle(rootNode))));
		//Create new internode
		const auto newRootNodeHandle = m_rootSkeleton.Extend(rootNodeHandle, false);
		const auto& oldRootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& newRootNode = m_rootSkeleton.RefNode(newRootNodeHandle);
		newRootNode.m_data = {};
		newRootNode.m_data.m_startAge = m_age;
		newRootNode.m_data.m_order = oldRootNode.m_data.m_order;
		newRootNode.m_data.m_lateral = false;
		//Set and apply tropisms
		auto desiredGlobalFront = desiredGlobalRotation * glm::vec3(0, 0, -1);
		auto desiredGlobalUp = desiredGlobalRotation * glm::vec3(0, 1, 0);
		newRootNode.m_data.m_verticalTropism = oldRootNode.m_data.m_verticalTropism;
		newRootNode.m_data.m_horizontalTropism = oldRootNode.m_data.m_horizontalTropism;
		auto horizontalDirection = desiredGlobalFront;
		horizontalDirection.y = 0;
		if (glm::length(horizontalDirection) == 0) {
			auto x = glm::linearRand(0.0f, 1.0f);
			horizontalDirection = glm::vec3(x, 0, 1.0f - x);
		}
		horizontalDirection = glm::normalize(horizontalDirection);
		ApplyTropism(horizontalDirection, newRootNode.m_data.m_horizontalTropism, desiredGlobalFront,
			desiredGlobalUp);
		ApplyTropism(m_currentGravityDirection, newRootNode.m_data.m_verticalTropism,
			desiredGlobalFront, desiredGlobalUp);
		if (oldRootNode.m_data.m_soilDensity == 0.0f) {
			ApplyTropism(m_currentGravityDirection, 0.1f, desiredGlobalFront,
				desiredGlobalUp);
		}
		newRootNode.m_data.m_vigorFlow.m_vigorRequirementWeight = oldRootNode.m_data.m_vigorFlow.m_vigorRequirementWeight;

		newRootNode.m_data.m_inhibitor = 0.0f;
		newRootNode.m_info.m_length = glm::clamp(extendLength, 0.0f, rootNodeLength);
		newRootNode.m_data.m_rootDistance = oldRootNode.m_data.m_rootDistance + newRootNode.m_info.m_length / rootGrowthParameters.m_rootNodeLength;
		newRootNode.m_info.m_thickness = rootGrowthParameters.m_endNodeThickness;
		newRootNode.m_info.m_globalRotation = glm::quatLookAt(desiredGlobalFront, desiredGlobalUp);
		newRootNode.m_data.m_localRotation =
			glm::inverse(oldRootNode.m_info.m_globalRotation) *
			newRootNode.m_info.m_globalRotation;

		if (extraLength > rootNodeLength) {
			float childAuxin = 0.0f;
			ElongateRoot(soilModel, extraLength - rootNodeLength, newRootNodeHandle, rootGrowthParameters, childAuxin);
			childAuxin *= rootGrowthParameters.m_apicalDominanceDistanceFactor;
			collectedAuxin += childAuxin;
			m_rootSkeleton.RefNode(newRootNodeHandle).m_data.m_inhibitor = childAuxin;
		}
		else {
			newRootNode.m_data.m_inhibitor = rootGrowthParameters.m_apicalDominance(newRootNode);
			collectedAuxin += newRootNode.m_data.m_inhibitor *= rootGrowthParameters.m_apicalDominanceDistanceFactor;
		}
	}
	else {
		//Otherwise, we add the inhibitor.
		collectedAuxin += rootGrowthParameters.m_apicalDominance(rootNode);
	}
	return graphChanged;
}

bool TreeModel::ElongateInternode(float extendLength, NodeHandle internodeHandle,
	const ShootGrowthController& shootGrowthController, float& collectedInhibitor) {
	bool graphChanged = false;
	auto& internode = m_shootSkeleton.RefNode(internodeHandle);
	const auto internodeLength = shootGrowthController.m_internodeLength;
	auto& internodeData = internode.m_data;
	const auto& internodeInfo = internode.m_info;
	internodeData.m_internodeLength += extendLength;
	const float extraLength = internodeData.m_internodeLength - internodeLength;
	auto& apicalBud = internodeData.m_buds.front();
	//If we need to add a new end node
	if (extraLength >= 0) {
		graphChanged = true;
		apicalBud.m_status = BudStatus::Died;
		internodeData.m_internodeLength = internodeLength;
		const auto desiredGlobalRotation = internodeInfo.m_globalRotation * apicalBud.m_localRotation;
		auto desiredGlobalFront = desiredGlobalRotation * glm::vec3(0, 0, -1);
		auto desiredGlobalUp = desiredGlobalRotation * glm::vec3(0, 1, 0);
		if (internodeHandle != 0)
		{
			ApplyTropism(-m_currentGravityDirection, shootGrowthController.m_gravitropism(internode), desiredGlobalFront,
				desiredGlobalUp);
			ApplyTropism(internodeData.m_lightDirection, shootGrowthController.m_phototropism(internode),
				desiredGlobalFront, desiredGlobalUp);
		}

		//Allocate Lateral bud for current internode
		{
			const auto lateralBudCount = shootGrowthController.m_lateralBudCount;
			const float turnAngle = glm::radians(360.0f / lateralBudCount);
			for (int i = 0; i < lateralBudCount; i++) {
				internodeData.m_buds.emplace_back();
				auto& lateralBud = internodeData.m_buds.back();
				lateralBud.m_type = BudType::Lateral;
				lateralBud.m_status = BudStatus::Dormant;
				lateralBud.m_localRotation = glm::vec3(glm::radians(shootGrowthController.m_branchingAngle(internode)), 0.0f,
					i * turnAngle);
			}
		}
		//Allocate Fruit bud for current internode
		{
			const auto fruitBudCount = shootGrowthController.m_fruitBudCount;
			for (int i = 0; i < fruitBudCount; i++) {
				internodeData.m_buds.emplace_back();
				auto& fruitBud = internodeData.m_buds.back();
				fruitBud.m_type = BudType::Fruit;
				fruitBud.m_status = BudStatus::Dormant;
				fruitBud.m_localRotation = glm::vec3(
					glm::radians(shootGrowthController.m_branchingAngle(internode)), 0.0f,
					glm::radians(glm::linearRand(0.0f, 360.0f)));
			}
		}
		//Allocate Leaf bud for current internode
		{
			const auto leafBudCount = shootGrowthController.m_leafBudCount;
			for (int i = 0; i < leafBudCount; i++) {
				internodeData.m_buds.emplace_back();
				auto& leafBud = internodeData.m_buds.back();
				//Hack: Leaf bud will be given vigor for the first time.
				leafBud.m_vigorSink.AddVigor(shootGrowthController.m_leafVigorRequirement);
				leafBud.m_type = BudType::Leaf;
				leafBud.m_status = BudStatus::Dormant;
				leafBud.m_localRotation = glm::vec3(
					glm::radians(shootGrowthController.m_branchingAngle(internode)), 0.0f,
					glm::radians(glm::linearRand(0.0f, 360.0f)));
			}
		}
		//Create new internode
		const auto newInternodeHandle = m_shootSkeleton.Extend(internodeHandle, false);
		const auto& oldInternode = m_shootSkeleton.RefNode(internodeHandle);
		auto& newInternode = m_shootSkeleton.RefNode(newInternodeHandle);
		newInternode.m_data = {};
		newInternode.m_data.m_startAge = m_age;
		newInternode.m_data.m_order = oldInternode.m_data.m_order;
		newInternode.m_data.m_lateral = false;
		newInternode.m_data.m_inhibitor = 0.0f;
		newInternode.m_data.m_internodeLength = glm::clamp(extendLength, 0.0f, internodeLength);
		newInternode.m_info.m_thickness = shootGrowthController.m_endNodeThickness;
		newInternode.m_info.m_globalRotation = glm::quatLookAt(desiredGlobalFront, desiredGlobalUp);
		newInternode.m_data.m_localRotation = newInternode.m_data.m_desiredLocalRotation =
			glm::inverse(oldInternode.m_info.m_globalRotation) *
			newInternode.m_info.m_globalRotation;
		//Allocate apical bud for new internode
		newInternode.m_data.m_buds.emplace_back();
		auto& newApicalBud = newInternode.m_data.m_buds.back();
		newApicalBud.m_type = BudType::Apical;
		newApicalBud.m_status = BudStatus::Dormant;
		newApicalBud.m_localRotation = glm::vec3(
			glm::radians(shootGrowthController.m_apicalAngle(newInternode)), 0.0f,
			glm::radians(shootGrowthController.m_rollAngle(newInternode)));
		if(internodeHandle != 0 && shootGrowthController.m_apicalInternodeKillProbability(newInternode) > glm::linearRand(0.0f, 1.0f))
		{
			newApicalBud.m_status = BudStatus::Removed;
		}
		if (extraLength > internodeLength) {
			float childInhibitor = 0.0f;
			ElongateInternode(extraLength - internodeLength, newInternodeHandle, shootGrowthController, childInhibitor);
			childInhibitor *= shootGrowthController.m_apicalDominanceDistanceFactor;
			collectedInhibitor += childInhibitor;
			m_shootSkeleton.RefNode(newInternodeHandle).m_data.m_inhibitor = childInhibitor;
		}
		else {
			newInternode.m_data.m_inhibitor = shootGrowthController.m_apicalDominance(newInternode);
			collectedInhibitor += newInternode.m_data.m_inhibitor *= shootGrowthController.m_apicalDominanceDistanceFactor;
		}
	}
	else {
		//Otherwise, we add the inhibitor.
		collectedInhibitor += shootGrowthController.m_apicalDominance(internode);
	}
	return graphChanged;
}


bool TreeModel::GrowRootNode(VoxelSoilModel& soilModel, NodeHandle rootNodeHandle, const RootGrowthController& rootGrowthParameters)
{
	bool graphChanged = false;
	{
		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& rootNodeData = rootNode.m_data;
		rootNodeData.m_inhibitor = 0;
		for (const auto& childHandle : rootNode.RefChildHandles()) {
			rootNodeData.m_inhibitor += m_rootSkeleton.RefNode(childHandle).m_data.m_inhibitor *
				rootGrowthParameters.m_apicalDominanceDistanceFactor;
		}
	}

	{

		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		//1. Elongate current node.
		const float availableMaintenanceVigor = rootNode.m_data.m_vigorSink.GetAvailableMaintenanceVigor();
		float availableDevelopmentalVigor = rootNode.m_data.m_vigorSink.GetAvailableDevelopmentalVigor();
		const float developmentVigor = rootNode.m_data.m_vigorSink.SubtractAllDevelopmentalVigor();

		if (rootNode.RefChildHandles().empty())
		{
			const float extendLength = developmentVigor / rootGrowthParameters.m_rootNodeVigorRequirement * rootGrowthParameters.m_rootNodeLength;
			//Remove development vigor from sink since it's used for elongation
			float collectedAuxin = 0.0f;
			const auto dd = rootGrowthParameters.m_apicalDominanceDistanceFactor;
			graphChanged = ElongateRoot(soilModel, extendLength, rootNodeHandle, rootGrowthParameters, collectedAuxin) || graphChanged;
			m_rootSkeleton.RefNode(rootNodeHandle).m_data.m_inhibitor += collectedAuxin * dd;

			const float maintenanceVigor = m_rootSkeleton.RefNode(rootNodeHandle).m_data.m_vigorSink.SubtractVigor(availableMaintenanceVigor);
		}
		else
		{
			//2. Form new shoot if necessary
			float branchingProb = m_rootNodeDevelopmentRate * m_currentDeltaTime * rootGrowthParameters.m_rootNodeGrowthRate * rootGrowthParameters.m_branchingProbability(rootNode);
			if (rootNode.m_data.m_inhibitor > 0.0f) branchingProb *= glm::exp(-rootNode.m_data.m_inhibitor);
			//More nitrite, more likely to form new shoot.
			if (branchingProb >= glm::linearRand(0.0f, 1.0f)) {
				const auto newRootNodeHandle = m_rootSkeleton.Extend(rootNodeHandle, true);
				auto& oldRootNode = m_rootSkeleton.RefNode(rootNodeHandle);
				auto& newRootNode = m_rootSkeleton.RefNode(newRootNodeHandle);
				newRootNode.m_data = {};
				newRootNode.m_data.m_startAge = m_age;
				newRootNode.m_data.m_order = oldRootNode.m_data.m_order + 1;
				newRootNode.m_data.m_lateral = true;
				//Assign new tropism for new shoot based on parent node. The tropism switching happens here.
				rootGrowthParameters.SetTropisms(oldRootNode, newRootNode);
				newRootNode.m_info.m_length = 0.0f;
				newRootNode.m_info.m_thickness = rootGrowthParameters.m_endNodeThickness;
				newRootNode.m_data.m_localRotation =
					glm::quat(glm::vec3(
						glm::radians(rootGrowthParameters.m_branchingAngle(newRootNode)),
						glm::radians(glm::linearRand(0.0f, 360.0f)), 0.0f));
				auto globalRotation = oldRootNode.m_info.m_globalRotation * newRootNode.m_data.m_localRotation;
				auto front = globalRotation * glm::vec3(0, 0, -1);
				auto up = globalRotation * glm::vec3(0, 1, 0);
				auto angleTowardsUp = glm::degrees(glm::acos(glm::dot(front, glm::vec3(0, 1, 0))));
				const float maxAngle = 60.0f;
				if (angleTowardsUp < maxAngle) {
					const glm::vec3 left = glm::cross(front, glm::vec3(0, -1, 0));
					front = glm::rotate(front, glm::radians(maxAngle - angleTowardsUp), left);

					up = glm::normalize(glm::cross(glm::cross(front, up), front));
					globalRotation = glm::quatLookAt(front, up);
					newRootNode.m_data.m_localRotation = glm::inverse(oldRootNode.m_info.m_globalRotation) * globalRotation;
					front = globalRotation * glm::vec3(0, 0, -1);
				}
				const float maintenanceVigor = m_rootSkeleton.RefNode(rootNodeHandle).m_data.m_vigorSink.SubtractVigor(availableMaintenanceVigor);
			}
		}


	}
	return graphChanged;
}

bool TreeModel::GrowInternode(ClimateModel& climateModel, NodeHandle internodeHandle, const ShootGrowthController& shootGrowthParameters, bool seasonality) {
	bool graphChanged = false;
	{
		auto& internode = m_shootSkeleton.RefNode(internodeHandle);
		auto& internodeData = internode.m_data;
		internodeData.m_inhibitor = 0;
		for (const auto& childHandle : internode.RefChildHandles()) {
			internodeData.m_inhibitor += m_shootSkeleton.RefNode(childHandle).m_data.m_inhibitor *
				shootGrowthParameters.m_apicalDominanceDistanceFactor;
		}
	}
	auto& buds = m_shootSkeleton.RefNode(internodeHandle).m_data.m_buds;
	for (auto& bud : buds) {
		auto& internode = m_shootSkeleton.RefNode(internodeHandle);
		auto& internodeData = internode.m_data;
		auto& internodeInfo = internode.m_info;

		//Calculate vigor used for maintenance and development.
		const float desiredMaintenanceVigor = bud.m_vigorSink.GetDesiredMaintenanceVigorRequirement();
		const float availableMaintenanceVigor = bud.m_vigorSink.GetAvailableMaintenanceVigor();
		const float availableDevelopmentVigor = bud.m_vigorSink.GetAvailableDevelopmentalVigor();
		const float maintenanceVigor = bud.m_vigorSink.SubtractVigor(availableMaintenanceVigor);
		if (desiredMaintenanceVigor != 0.0f && availableMaintenanceVigor < desiredMaintenanceVigor) {
			bud.m_reproductiveModule.m_health = glm::clamp(bud.m_reproductiveModule.m_health * availableMaintenanceVigor / desiredMaintenanceVigor, 0.0f, 1.0f);
		}
		if (bud.m_type == BudType::Apical && bud.m_status == BudStatus::Dormant) {
			const float developmentalVigor = bud.m_vigorSink.SubtractVigor(availableDevelopmentVigor);
			float elongateLength = 0.0f;
			if (m_treeGrowthSettings.m_useSpaceColonization) {
				if (m_shootSkeleton.m_data.m_maxMarkerCount > 0) elongateLength = static_cast<float>(bud.m_markerCount) / m_shootSkeleton.m_data.m_maxMarkerCount * shootGrowthParameters.m_internodeLength;
			}
			else
			{
				elongateLength = developmentalVigor / shootGrowthParameters.m_internodeVigorRequirement * shootGrowthParameters.m_internodeLength;
			}
			if (internodeData.m_spaceOccupancy > shootGrowthParameters.m_maxSpaceOccupancy) elongateLength = 0.0f;

			//Use up the vigor stored in this bud.
			float collectedInhibitor = 0.0f;
			const auto dd = shootGrowthParameters.m_apicalDominanceDistanceFactor;
			graphChanged = ElongateInternode(elongateLength, internodeHandle, shootGrowthParameters, collectedInhibitor) || graphChanged;
			m_shootSkeleton.RefNode(internodeHandle).m_data.m_inhibitor += collectedInhibitor * dd;
		}
		if (bud.m_type == BudType::Lateral && bud.m_status == BudStatus::Dormant) {
			float flushProbability = 0.0f;
			if (m_treeGrowthSettings.m_useSpaceColonization) {
				if (m_shootSkeleton.m_data.m_maxMarkerCount > 0) flushProbability = static_cast<float>(bud.m_markerCount) / m_shootSkeleton.m_data.m_maxMarkerCount * shootGrowthParameters.m_lateralBudFlushProbability(internode);
			}
			else
			{
				flushProbability = m_internodeDevelopmentRate * m_currentDeltaTime * shootGrowthParameters.m_internodeGrowthRate * shootGrowthParameters.m_lateralBudFlushProbability(internode);
			}
			if (internodeData.m_spaceOccupancy > shootGrowthParameters.m_maxSpaceOccupancy) flushProbability = 0.0f;
			if (flushProbability >= glm::linearRand(0.0f, 1.0f)) {
				graphChanged = true;
				bud.m_status = BudStatus::Flushed;
				//Prepare information for new internode
				auto desiredGlobalRotation = internodeInfo.m_globalRotation * bud.m_localRotation;
				auto desiredGlobalFront = desiredGlobalRotation * glm::vec3(0, 0, -1);
				auto desiredGlobalUp = desiredGlobalRotation * glm::vec3(0, 1, 0);
				ApplyTropism(-m_currentGravityDirection, shootGrowthParameters.m_gravitropism(internode), desiredGlobalFront,
					desiredGlobalUp);
				ApplyTropism(internodeData.m_lightDirection, shootGrowthParameters.m_phototropism(internode),
					desiredGlobalFront, desiredGlobalUp);
				//Create new internode
				const auto newInternodeHandle = m_shootSkeleton.Extend(internodeHandle, true);
				auto& oldInternode = m_shootSkeleton.RefNode(internodeHandle);
				auto& newInternode = m_shootSkeleton.RefNode(newInternodeHandle);
				newInternode.m_data = {};
				newInternode.m_data.m_startAge = m_age;
				newInternode.m_data.m_order = oldInternode.m_data.m_order + 1;
				newInternode.m_data.m_lateral = true;
				newInternode.m_data.m_internodeLength = 0.0f;
				newInternode.m_info.m_thickness = shootGrowthParameters.m_endNodeThickness;
				newInternode.m_data.m_localRotation = newInternode.m_data.m_desiredLocalRotation =
					glm::inverse(oldInternode.m_info.m_globalRotation) *
					glm::quatLookAt(desiredGlobalFront, desiredGlobalUp);
				//Allocate apical bud
				newInternode.m_data.m_buds.emplace_back();
				auto& apicalBud = newInternode.m_data.m_buds.back();
				apicalBud.m_type = BudType::Apical;
				apicalBud.m_status = BudStatus::Dormant;
				apicalBud.m_localRotation = glm::vec3(
					glm::radians(shootGrowthParameters.m_apicalAngle(newInternode)), 0.0f,
					glm::radians(shootGrowthParameters.m_rollAngle(newInternode)));
			}
		}
		else if (bud.m_type == BudType::Fruit)
		{
			if (!seasonality)
			{
				const float maxMaturityIncrease = availableDevelopmentVigor / shootGrowthParameters.m_fruitVigorRequirement;
				const auto developmentVigor = bud.m_vigorSink.SubtractVigor(maxMaturityIncrease * shootGrowthParameters.m_fruitVigorRequirement);
			}
			else if (bud.m_status == BudStatus::Dormant) {
				const float flushProbability = m_currentDeltaTime * shootGrowthParameters.m_fruitBudFlushingProbability(internode);
				if (flushProbability >= glm::linearRand(0.0f, 1.0f))
				{
					bud.m_status = BudStatus::Flushed;
				}
			}
			else if (bud.m_status == BudStatus::Flushed)
			{
				//Make the fruit larger;
				const float maxMaturityIncrease = availableDevelopmentVigor / shootGrowthParameters.m_fruitVigorRequirement;
				const float maturityIncrease = glm::min(maxMaturityIncrease, glm::min(m_currentDeltaTime * shootGrowthParameters.m_fruitGrowthRate, 1.0f - bud.m_reproductiveModule.m_maturity));
				bud.m_reproductiveModule.m_maturity += maturityIncrease;
				const auto developmentVigor = bud.m_vigorSink.SubtractVigor(maturityIncrease * shootGrowthParameters.m_fruitVigorRequirement);
				auto fruitSize = shootGrowthParameters.m_maxFruitSize * glm::sqrt(bud.m_reproductiveModule.m_maturity);
				float angle = glm::radians(glm::linearRand(0.0f, 360.0f));
				glm::quat rotation = internodeData.m_desiredLocalRotation * bud.m_localRotation;
				auto up = rotation * glm::vec3(0, 1, 0);
				auto front = rotation * glm::vec3(0, 0, -1);
				ApplyTropism(internodeData.m_lightDirection, 0.3f, up, front);
				rotation = glm::quatLookAt(front, up);
				auto fruitPosition = internodeInfo.m_globalPosition + front * (fruitSize.z * 1.5f);
				bud.m_reproductiveModule.m_transform = glm::translate(fruitPosition) * glm::mat4_cast(glm::quat(glm::vec3(0.0f))) * glm::scale(fruitSize);

				bud.m_reproductiveModule.m_health -= m_currentDeltaTime * shootGrowthParameters.m_fruitDamage(internode);
				bud.m_reproductiveModule.m_health = glm::clamp(bud.m_reproductiveModule.m_health, 0.0f, 1.0f);

				//Handle fruit drop here.
				if (bud.m_reproductiveModule.m_maturity >= 0.95f || bud.m_reproductiveModule.m_health <= 0.05f)
				{
					auto dropProbability = m_currentDeltaTime * shootGrowthParameters.m_fruitFallProbability(internode);
					if (dropProbability >= glm::linearRand(0.0f, 1.0f))
					{
						bud.m_status = BudStatus::Died;
						m_shootSkeleton.m_data.m_droppedFruits.emplace_back(bud.m_reproductiveModule);
						bud.m_reproductiveModule.Reset();
					}
				}

			}
		}
		else if (bud.m_type == BudType::Leaf)
		{
			if (!seasonality)
			{
				const float maxMaturityIncrease = availableDevelopmentVigor / shootGrowthParameters.m_leafVigorRequirement;
				const auto developmentVigor = bud.m_vigorSink.SubtractVigor(maxMaturityIncrease * shootGrowthParameters.m_leafVigorRequirement);
			}
			else if (bud.m_status == BudStatus::Dormant) {
				const float flushProbability = m_currentDeltaTime * shootGrowthParameters.m_leafBudFlushingProbability(internode);
				if (flushProbability >= glm::linearRand(0.0f, 1.0f))
				{
					bud.m_status = BudStatus::Flushed;
				}
			}
			else if (bud.m_status == BudStatus::Flushed)
			{
				//Make the leaf larger
				const float maxMaturityIncrease = availableDevelopmentVigor / shootGrowthParameters.m_leafVigorRequirement;
				const float maturityIncrease = glm::min(maxMaturityIncrease, glm::min(m_currentDeltaTime * shootGrowthParameters.m_leafGrowthRate, 1.0f - bud.m_reproductiveModule.m_maturity));
				bud.m_reproductiveModule.m_maturity += maturityIncrease;
				const auto developmentVigor = bud.m_vigorSink.SubtractVigor(maturityIncrease * shootGrowthParameters.m_leafVigorRequirement);
				auto leafSize = shootGrowthParameters.m_maxLeafSize * glm::sqrt(bud.m_reproductiveModule.m_maturity);
				glm::quat rotation = internodeData.m_desiredLocalRotation * bud.m_localRotation;
				auto up = rotation * glm::vec3(0, 1, 0);
				auto front = rotation * glm::vec3(0, 0, -1);
				ApplyTropism(internodeData.m_lightDirection, 0.3f, up, front);
				rotation = glm::quatLookAt(front, up);
				auto foliagePosition = internodeInfo.m_globalPosition + front * (leafSize.z * 1.5f);
				bud.m_reproductiveModule.m_transform = glm::translate(foliagePosition) * glm::mat4_cast(rotation) * glm::scale(leafSize);

				bud.m_reproductiveModule.m_health -= m_currentDeltaTime * shootGrowthParameters.m_leafDamage(internode);
				bud.m_reproductiveModule.m_health = glm::clamp(bud.m_reproductiveModule.m_health, 0.0f, 1.0f);

				//Handle leaf drop here.
				if (bud.m_reproductiveModule.m_health <= 0.05f)
				{
					auto dropProbability = m_currentDeltaTime * shootGrowthParameters.m_leafFallProbability(internode);
					if (dropProbability >= glm::linearRand(0.0f, 1.0f))
					{
						bud.m_status = BudStatus::Died;
						m_shootSkeleton.m_data.m_droppedLeaves.emplace_back(bud.m_reproductiveModule);
						bud.m_reproductiveModule.Reset();
					}
				}
			}
		}
	}
	return graphChanged;
}

void TreeModel::CalculateThickness(NodeHandle rootNodeHandle, const RootGrowthController& rootGrowthParameters)
{
	auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
	auto& rootNodeData = rootNode.m_data;
	auto& rootNodeInfo = rootNode.m_info;
	rootNodeData.m_descendentTotalBiomass = 0;
	float maxDistanceToAnyBranchEnd = 0;
	float childThicknessCollection = 0.0f;
	//std::set<float> thicknessCollection;
	int maxChildHandle = -1;
	//float maxChildBiomass = 999.f;
	int minChildOrder = 999;
	for (const auto& i : rootNode.RefChildHandles()) {
		const auto& childRootNode = m_rootSkeleton.RefNode(i);
		const float childMaxDistanceToAnyBranchEnd =
			childRootNode.m_data.m_maxDistanceToAnyBranchEnd +
			childRootNode.m_info.m_length / rootGrowthParameters.m_rootNodeLength;
		maxDistanceToAnyBranchEnd = glm::max(maxDistanceToAnyBranchEnd, childMaxDistanceToAnyBranchEnd);
		childThicknessCollection += glm::pow(childRootNode.m_info.m_thickness,
			1.0f / rootGrowthParameters.m_thicknessAccumulationFactor);
		//thicknessCollection.emplace();
		if (childRootNode.m_data.m_order > minChildOrder)
		{
			minChildOrder = childRootNode.m_data.m_order;
			maxChildHandle = i;
		}
	}

	//int addedIndex = 0;
	//for (auto i = thicknessCollection.begin(); i != thicknessCollection.end(); ++i)
	//{
	//	childThicknessCollection += *i;
	//	addedIndex++;
		//if (addedIndex > 1) break;
	//}

	childThicknessCollection += rootGrowthParameters.m_thicknessAccumulateAgeFactor * rootGrowthParameters.m_endNodeThickness * rootGrowthParameters.m_rootNodeGrowthRate * (m_age - rootNodeData.m_startAge);

	rootNodeData.m_maxDistanceToAnyBranchEnd = maxDistanceToAnyBranchEnd;
	if (childThicknessCollection != 0.0f) {
		const auto newThickness = glm::pow(childThicknessCollection,
			rootGrowthParameters.m_thicknessAccumulationFactor);
		rootNodeInfo.m_thickness = glm::max(rootNodeInfo.m_thickness, newThickness);
	}
	else
	{
		rootNodeInfo.m_thickness = glm::max(rootNodeInfo.m_thickness, rootGrowthParameters.m_endNodeThickness);
	}

	rootNodeData.m_biomass =
		rootNodeInfo.m_thickness / rootGrowthParameters.m_endNodeThickness * rootNodeInfo.m_length /
		rootGrowthParameters.m_rootNodeLength;
	for (const auto& i : rootNode.RefChildHandles()) {
		auto& childRootNode = m_rootSkeleton.RefNode(i);
		rootNodeData.m_descendentTotalBiomass +=
			childRootNode.m_data.m_descendentTotalBiomass +
			childRootNode.m_data.m_biomass;
		childRootNode.m_data.m_isMaxChild = i == maxChildHandle;
	}
}

bool TreeModel::PruneRootNodes(const RootGrowthController& rootGrowthParameters)
{
	return false;
}

void TreeModel::AggregateInternodeVigorRequirement(const ShootGrowthController& shootGrowthParameters, NodeHandle baseInternodeHandle)
{
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		auto& internode = m_shootSkeleton.RefNode(*it);
		auto& internodeData = internode.m_data;
		if (!internode.IsEndNode()) {
			//If current node is not end node
			for (const auto& i : internode.RefChildHandles()) {
				const auto& childInternode = m_shootSkeleton.PeekNode(i);
				internodeData.m_vigorFlow.m_subtreeVigorRequirementWeight +=
					shootGrowthParameters.m_vigorRequirementAggregateLoss *
					(childInternode.m_data.m_vigorFlow.m_vigorRequirementWeight
						+ childInternode.m_data.m_vigorFlow.m_subtreeVigorRequirementWeight);
			}
		}
		if (*it == baseInternodeHandle) return;
	}
}

void TreeModel::AggregateRootVigorRequirement(const RootGrowthController& rootGrowthParameters, NodeHandle baseRootNodeHandle)
{
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();

	for (auto it = sortedRootNodeList.rbegin(); it != sortedRootNodeList.rend(); ++it) {
		auto& rootNode = m_rootSkeleton.RefNode(*it);
		auto& rootNodeData = rootNode.m_data;
		rootNodeData.m_vigorFlow.m_subtreeVigorRequirementWeight = 0.0f;
		if (!rootNode.IsEndNode()) {
			//If current node is not end node
			for (const auto& i : rootNode.RefChildHandles()) {
				const auto& childInternode = m_rootSkeleton.RefNode(i);
				rootNodeData.m_vigorFlow.m_subtreeVigorRequirementWeight +=
					rootGrowthParameters.m_vigorRequirementAggregateLoss *
					(childInternode.m_data.m_vigorFlow.m_vigorRequirementWeight + childInternode.m_data.m_vigorFlow.m_subtreeVigorRequirementWeight);
			}
		}
		if (*it == baseRootNodeHandle) return;
	}
}

void TreeModel::AllocateShootVigor(const float vigor, const NodeHandle baseInternodeHandle, const std::vector<NodeHandle>& sortedInternodeList, const ShootGrowthRequirement& shootGrowthRequirement, const ShootGrowthController& shootGrowthParameters)
{
	//Go from rooting point to all end nodes
	const float apicalControl = shootGrowthParameters.m_apicalControl;
	float remainingVigor = vigor;

	const float leafMaintenanceVigor = glm::min(remainingVigor, shootGrowthRequirement.m_leafMaintenanceVigor);
	remainingVigor -= leafMaintenanceVigor;
	float leafMaintenanceVigorFillingRate = 0.0f;
	if (shootGrowthRequirement.m_leafMaintenanceVigor != 0.0f)
		leafMaintenanceVigorFillingRate = leafMaintenanceVigor / shootGrowthRequirement.m_leafMaintenanceVigor;

	const float leafDevelopmentVigor = glm::min(remainingVigor, shootGrowthRequirement.m_leafDevelopmentalVigor);
	remainingVigor -= leafDevelopmentVigor;
	float leafDevelopmentVigorFillingRate = 0.0f;
	if (shootGrowthRequirement.m_leafDevelopmentalVigor != 0.0f)
		leafDevelopmentVigorFillingRate = leafDevelopmentVigor / shootGrowthRequirement.m_leafDevelopmentalVigor;

	const float fruitMaintenanceVigor = glm::min(remainingVigor, shootGrowthRequirement.m_fruitMaintenanceVigor);
	remainingVigor -= fruitMaintenanceVigor;
	float fruitMaintenanceVigorFillingRate = 0.0f;
	if (shootGrowthRequirement.m_fruitMaintenanceVigor != 0.0f)
		fruitMaintenanceVigorFillingRate = fruitMaintenanceVigor / shootGrowthRequirement.m_fruitMaintenanceVigor;

	const float fruitDevelopmentVigor = glm::min(remainingVigor, shootGrowthRequirement.m_fruitDevelopmentalVigor);
	remainingVigor -= fruitDevelopmentVigor;
	float fruitDevelopmentVigorFillingRate = 0.0f;
	if (shootGrowthRequirement.m_fruitDevelopmentalVigor != 0.0f)
		fruitDevelopmentVigorFillingRate = fruitDevelopmentVigor / shootGrowthRequirement.m_fruitDevelopmentalVigor;

	const float nodeDevelopmentVigor = glm::min(remainingVigor, shootGrowthRequirement.m_nodeDevelopmentalVigor);
	if (shootGrowthRequirement.m_nodeDevelopmentalVigor != 0.0f) {
		m_internodeDevelopmentRate = nodeDevelopmentVigor / shootGrowthRequirement.m_nodeDevelopmentalVigor;
	}

	for (const auto& internodeHandle : sortedInternodeList) {
		auto& internode = m_shootSkeleton.RefNode(internodeHandle);
		auto& internodeData = internode.m_data;
		auto& internodeVigorFlow = internodeData.m_vigorFlow;
		//1. Allocate maintenance vigor first
		for (auto& bud : internodeData.m_buds) {
			switch (bud.m_type)
			{
			case BudType::Leaf:
			{
				bud.m_vigorSink.AddVigor(leafMaintenanceVigorFillingRate * bud.m_vigorSink.GetMaintenanceVigorRequirement());
				bud.m_vigorSink.AddVigor(leafDevelopmentVigorFillingRate * bud.m_vigorSink.GetMaxVigorRequirement());
			}break;
			case BudType::Fruit:
			{
				bud.m_vigorSink.AddVigor(fruitMaintenanceVigorFillingRate * bud.m_vigorSink.GetMaintenanceVigorRequirement());
				bud.m_vigorSink.AddVigor(fruitDevelopmentVigorFillingRate * bud.m_vigorSink.GetMaxVigorRequirement());
			}break;
			default:break;
			}

		}
		//2. Allocate development vigor for structural growth
		//If this is the first node (node at the rooting point)
		if (internode.GetHandle() == baseInternodeHandle) {
			internodeVigorFlow.m_allocatedVigor = 0.0f;
			internodeVigorFlow.m_subTreeAllocatedVigor = 0.0f;
			if (shootGrowthRequirement.m_nodeDevelopmentalVigor != 0.0f) {
				const float totalRequirement = internodeVigorFlow.m_vigorRequirementWeight + internodeVigorFlow.m_subtreeVigorRequirementWeight;
				if (totalRequirement != 0.0f) {
					//The root internode firstly extract it's own resources needed for itself.
					internodeVigorFlow.m_allocatedVigor = nodeDevelopmentVigor * internodeVigorFlow.m_vigorRequirementWeight / totalRequirement;
				}
				//The rest resource will be distributed to the descendants. 
				internodeVigorFlow.m_subTreeAllocatedVigor = nodeDevelopmentVigor - internodeVigorFlow.m_allocatedVigor;
			}

		}
		//The buds will get its own resources
		for (auto& bud : internodeData.m_buds) {
			if (bud.m_type == BudType::Apical && internodeVigorFlow.m_vigorRequirementWeight != 0.0f) {
				//The vigor gets allocated and stored eventually into the buds
				const float budAllocatedVigor = internodeVigorFlow.m_allocatedVigor *
					bud.m_vigorSink.GetMaxVigorRequirement() / internodeVigorFlow.m_vigorRequirementWeight;
				bud.m_vigorSink.AddVigor(budAllocatedVigor);
			}
		}

		if (internodeVigorFlow.m_subTreeAllocatedVigor != 0.0f) {
			float childDevelopmentalVigorRequirementWeightSum = 0.0f;
			for (const auto& i : internode.RefChildHandles()) {
				const auto& childInternode = m_shootSkeleton.RefNode(i);
				const auto& childInternodeData = childInternode.m_data;
				auto& childInternodeVigorFlow = childInternodeData.m_vigorFlow;
				float childDevelopmentalVigorRequirementWeight = 0.0f;
				if (internodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f)childDevelopmentalVigorRequirementWeight = (childInternodeVigorFlow.m_vigorRequirementWeight + childInternodeVigorFlow.m_subtreeVigorRequirementWeight)
					/ internodeVigorFlow.m_subtreeVigorRequirementWeight;
				//Perform Apical control here.
				if (childInternodeData.m_isMaxChild) childDevelopmentalVigorRequirementWeight *= apicalControl;
				childDevelopmentalVigorRequirementWeightSum += childDevelopmentalVigorRequirementWeight;
			}
			for (const auto& i : internode.RefChildHandles()) {
				auto& childInternode = m_shootSkeleton.RefNode(i);
				auto& childInternodeData = childInternode.m_data;
				auto& childInternodeVigorFlow = childInternodeData.m_vigorFlow;
				float childDevelopmentalVigorRequirementWeight = 0.0f;
				if (internodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f)
					childDevelopmentalVigorRequirementWeight = (childInternodeVigorFlow.m_vigorRequirementWeight + childInternodeVigorFlow.m_subtreeVigorRequirementWeight)
					/ internodeVigorFlow.m_subtreeVigorRequirementWeight;

				//Re-perform apical control.
				if (childInternodeData.m_isMaxChild) childDevelopmentalVigorRequirementWeight *= apicalControl;

				//Calculate total amount of development vigor belongs to this child from internode received vigor for its children.
				float childTotalAllocatedDevelopmentVigor = 0.0f;
				if (childDevelopmentalVigorRequirementWeightSum != 0.0f) childTotalAllocatedDevelopmentVigor = internodeVigorFlow.m_subTreeAllocatedVigor *
					childDevelopmentalVigorRequirementWeight / childDevelopmentalVigorRequirementWeightSum;

				//Calculate allocated vigor.
				childInternodeVigorFlow.m_allocatedVigor = 0.0f;
				if (childInternodeVigorFlow.m_vigorRequirementWeight + childInternodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f) {
					childInternodeVigorFlow.m_allocatedVigor += childTotalAllocatedDevelopmentVigor *
						childInternodeVigorFlow.m_vigorRequirementWeight
						/ (childInternodeVigorFlow.m_vigorRequirementWeight + childInternodeVigorFlow.m_subtreeVigorRequirementWeight);
				}
				childInternodeVigorFlow.m_subTreeAllocatedVigor = childTotalAllocatedDevelopmentVigor - childInternodeVigorFlow.m_allocatedVigor;
			}
		}
		else
		{
			for (const auto& i : internode.RefChildHandles())
			{
				auto& childInternode = m_shootSkeleton.RefNode(i);
				auto& childInternodeData = childInternode.m_data;
				auto& childInternodeVigorFlow = childInternodeData.m_vigorFlow;
				childInternodeVigorFlow.m_allocatedVigor = childInternodeVigorFlow.m_subTreeAllocatedVigor = 0.0f;
			}
		}
	}
}

void TreeModel::CalculateThicknessAndSagging(NodeHandle internodeHandle,
	const ShootGrowthController& shootGrowthParameters) {
	auto& internode = m_shootSkeleton.RefNode(internodeHandle);
	auto& internodeData = internode.m_data;
	auto& internodeInfo = internode.m_info;
	internodeData.m_descendentTotalBiomass = internodeData.m_biomass = 0.0f;
	float maxDistanceToAnyBranchEnd = 0;
	float childThicknessCollection = 0.0f;
	internodeInfo.m_endDistance = 0.0f;
	int maxChildHandle = -1;
	//float maxChildBiomass = 999.f;
	int minChildOrder = 999;
	for (const auto& i : internode.RefChildHandles()) {
		const auto& childInternode = m_shootSkeleton.PeekNode(i);
		const float childMaxDistanceToAnyBranchEnd =
			childInternode.m_info.m_endDistance +
			childInternode.m_data.m_internodeLength;
		maxDistanceToAnyBranchEnd = glm::max(maxDistanceToAnyBranchEnd, childMaxDistanceToAnyBranchEnd);

		childThicknessCollection += glm::pow(childInternode.m_info.m_thickness,
			1.0f / shootGrowthParameters.m_thicknessAccumulationFactor);

		if (childInternode.m_data.m_order < minChildOrder)
		{
			minChildOrder = childInternode.m_data.m_order;
			maxChildHandle = i;
		}
	}
	childThicknessCollection += shootGrowthParameters.m_thicknessAccumulateAgeFactor * shootGrowthParameters.m_endNodeThickness * shootGrowthParameters.m_internodeGrowthRate * (m_age - internodeData.m_startAge);


	internodeInfo.m_endDistance = maxDistanceToAnyBranchEnd;
	if (childThicknessCollection != 0.0f) {
		internodeInfo.m_thickness = glm::max(internodeInfo.m_thickness, glm::pow(childThicknessCollection,
			shootGrowthParameters.m_thicknessAccumulationFactor));
	}
	else
	{
		internodeInfo.m_thickness = glm::max(internodeInfo.m_thickness, shootGrowthParameters.m_endNodeThickness);
	}

	internodeData.m_biomass =
		internodeInfo.m_thickness / shootGrowthParameters.m_endNodeThickness * internodeData.m_internodeLength /
		shootGrowthParameters.m_internodeLength;
	for (const auto& i : internode.RefChildHandles()) {
		auto& childInternode = m_shootSkeleton.RefNode(i);
		internodeData.m_descendentTotalBiomass +=
			childInternode.m_data.m_descendentTotalBiomass +
			childInternode.m_data.m_biomass;
		childInternode.m_data.m_isMaxChild = i == maxChildHandle;
	}
	internodeData.m_sagging = shootGrowthParameters.m_sagging(internode);
}

void TreeModel::CalculateVigorRequirement(const ShootGrowthController& shootGrowthParameters, ShootGrowthRequirement& newTreeGrowthNutrientsRequirement) {

	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	for (const auto& internodeHandle : sortedInternodeList) {
		auto& internode = m_shootSkeleton.RefNode(internodeHandle);
		auto& internodeData = internode.m_data;
		auto& internodeVigorFlow = internodeData.m_vigorFlow;
		internodeVigorFlow.m_vigorRequirementWeight = 0.0f;
		internodeVigorFlow.m_subtreeVigorRequirementWeight = 0.0f;
		for (auto& bud : internodeData.m_buds) {
			bud.m_vigorSink.SetDesiredDevelopmentalVigorRequirement(0.0f);
			bud.m_vigorSink.SetDesiredMaintenanceVigorRequirement(0.0f);
			if (bud.m_status == BudStatus::Died || bud.m_status == BudStatus::Removed) {
				continue;
			}
			switch (bud.m_type) {
			case BudType::Apical: {
				if (bud.m_status == BudStatus::Dormant) {
					//Elongation
					bud.m_vigorSink.SetDesiredDevelopmentalVigorRequirement(m_currentDeltaTime * shootGrowthParameters.m_internodeGrowthRate * shootGrowthParameters.m_internodeVigorRequirement * internodeData.m_growthPotential);
					newTreeGrowthNutrientsRequirement.m_nodeDevelopmentalVigor += bud.m_vigorSink.GetMaxVigorRequirement();
					//Collect requirement for internode. The internode doesn't has it's own requirement for now since we consider it as simple pipes
					//that only perform transportation. However this can be change in future.
					internodeVigorFlow.m_vigorRequirementWeight += bud.m_vigorSink.GetDesiredDevelopmentalVigorRequirement();
				}
			}break;
			case BudType::Leaf: {
				if (bud.m_status == BudStatus::Dormant)
				{
					//No requirement since the lateral bud only gets activated and turned into new shoot.
					//We can make use of the development vigor for bud flushing probability here in future.
					bud.m_vigorSink.SetDesiredMaintenanceVigorRequirement(0.0f);
					newTreeGrowthNutrientsRequirement.m_leafMaintenanceVigor += bud.m_vigorSink.GetMaintenanceVigorRequirement();

				}
				else if (bud.m_status == BudStatus::Flushed)
				{
					//The maintenance vigor requirement is related to the size and the drought factor of the leaf.
					bud.m_vigorSink.SetDesiredDevelopmentalVigorRequirement(shootGrowthParameters.m_leafVigorRequirement * bud.m_reproductiveModule.m_health);
					newTreeGrowthNutrientsRequirement.m_leafDevelopmentalVigor += bud.m_vigorSink.GetMaxVigorRequirement();
				}
			}break;
			case BudType::Fruit: {
				if (bud.m_status == BudStatus::Dormant)
				{
					//No requirement since the lateral bud only gets activated and turned into new shoot.
					//We can make use of the development vigor for bud flushing probability here in future.
					bud.m_vigorSink.SetDesiredMaintenanceVigorRequirement(0.0f);
					newTreeGrowthNutrientsRequirement.m_fruitDevelopmentalVigor += bud.m_vigorSink.GetMaintenanceVigorRequirement();
				}
				else if (bud.m_status == BudStatus::Flushed)
				{
					//The maintenance vigor requirement is related to the volume and the drought factor of the fruit.
					bud.m_vigorSink.SetDesiredDevelopmentalVigorRequirement(shootGrowthParameters.m_fruitVigorRequirement * bud.m_reproductiveModule.m_health);
					newTreeGrowthNutrientsRequirement.m_fruitDevelopmentalVigor += bud.m_vigorSink.GetMaxVigorRequirement();
				}
			}break;
			default: break;
			}

		}
	}
}

void TreeModel::Clear() {
	m_shootSkeleton = {};
	m_rootSkeleton = {};
	m_history = {};
	m_initialized = false;

	if (m_treeGrowthSettings.m_useSpaceColonization && !m_treeGrowthSettings.m_spaceColonizationAutoResize)
	{
		m_treeOccupancyGrid.ResetMarkers();
	}
	else
	{
		m_treeOccupancyGrid = {};
	}

	m_age = 0;
	m_iteration = 0;
}

int TreeModel::GetLeafCount() const
{
	return m_leafCount;
}

int TreeModel::GetFruitCount() const
{
	return m_fruitCount;
}

int TreeModel::GetFineRootCount() const
{
	return m_fineRootCount;
}

bool TreeModel::PruneInternodes(const glm::mat4& globalTransform, ClimateModel& climateModel, const ShootGrowthController& shootGrowthParameters) {

	const auto maxDistance = m_shootSkeleton.PeekNode(0).m_info.m_endDistance;
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	bool anyInternodePruned = false;

	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		const auto internodeHandle = *it;
		if (m_shootSkeleton.PeekNode(internodeHandle).IsRecycled()) continue;
		if (internodeHandle == 0) continue;
		const auto& internode = m_shootSkeleton.PeekNode(*it);
		if (internode.m_info.m_globalPosition.y <= 0.05f && internode.m_data.m_order != 0)
		{
			auto handleWalker = internodeHandle;
			int i = 0;
			while(i < 4 && handleWalker != -1 && m_shootSkeleton.PeekNode(handleWalker).IsApical())
			{
				handleWalker = m_shootSkeleton.PeekNode(handleWalker).GetParentHandle();
				i++;
			}
			if (handleWalker != -1) {
				auto& targetInternode = m_shootSkeleton.PeekNode(handleWalker);
				if(targetInternode.m_data.m_order != 0)
				{
					PruneInternode(handleWalker);
					anyInternodePruned = true;
				}
			}
		}
	}

	if (anyInternodePruned) m_shootSkeleton.SortLists();

	for (const auto& internodeHandle : sortedInternodeList) {
		if (m_shootSkeleton.PeekNode(internodeHandle).IsRecycled()) continue;
		if(internodeHandle == 0) continue;
		const auto& internode = m_shootSkeleton.PeekNode(internodeHandle);
		//Pruning here.
		bool pruning = false;
		const float pruningProbability = m_currentDeltaTime * shootGrowthParameters.m_pruningFactor(m_currentDeltaTime, internode);
		if (pruningProbability > glm::linearRand(0.0f, 1.0f)) pruning = true;
		bool lowBranchPruning = false;
		if (!pruning && maxDistance > 5.0f * shootGrowthParameters.m_internodeLength && internode.m_data.m_order == 1 &&
			(internode.m_info.m_rootDistance / maxDistance) < shootGrowthParameters.m_lowBranchPruning) {
			const auto parentHandle = internode.GetParentHandle();
			if (parentHandle != -1) {
				const auto& parent = m_shootSkeleton.PeekNode(parentHandle);
				if (shootGrowthParameters.m_lowBranchPruningThicknessFactor == 0.0f || internode.m_info.m_thickness / parent.m_info.m_thickness < shootGrowthParameters.m_lowBranchPruningThicknessFactor) lowBranchPruning = true;
			}

		}
		bool pruneByCrownShyness = false;
		if (!pruning && m_crownShynessDistance > 0.f && internode.IsEndNode()) {
			//const glm::vec3 startPosition = globalTransform * glm::vec4(internode.m_info.m_globalPosition, 1.0f);
			const glm::vec3 endPosition = globalTransform * glm::vec4(internode.m_info.GetGlobalEndPosition(), 1.0f);
			climateModel.m_environmentGrid.m_voxel.ForEach(endPosition, m_crownShynessDistance * 2.0f, [&](EnvironmentVoxel& data)
				{
					if (pruneByCrownShyness) return;
					for (const auto& i : data.m_internodeVoxelRegistrations)
					{
						if (i.m_treeModelIndex == m_index) continue;
						if (glm::distance(endPosition, i.m_position) < m_crownShynessDistance) pruneByCrownShyness = true;
					}
				}
			);
		}
		if (pruning || pruneByCrownShyness || lowBranchPruning)
		{
			PruneInternode(internodeHandle);
			anyInternodePruned = true;
		}
	}
	return anyInternodePruned;
}

void TreeModel::SampleNitrite(const glm::mat4& globalTransform, VoxelSoilModel& soilModel)
{
	m_rootSkeleton.m_data.m_rootFlux.m_nitrite = 0.0f;
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
	for (const auto& rootNodeHandle : sortedRootNodeList) {
		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& rootNodeInfo = rootNode.m_info;
		auto worldSpacePosition = globalTransform * glm::translate(rootNodeInfo.m_globalPosition)[3];
		if (m_treeGrowthSettings.m_collectNitrite) {
			rootNode.m_data.m_nitrite = soilModel.IntegrateNutrient(worldSpacePosition, 0.2f);
			m_rootSkeleton.m_data.m_rootFlux.m_nitrite += rootNode.m_data.m_nitrite;
		}
		else
		{
			m_rootSkeleton.m_data.m_rootFlux.m_nitrite += 1.0f;
		}
	}
}



void TreeModel::AllocateRootVigor(const RootGrowthController& rootGrowthParameters)
{
	//For how this works, refer to AllocateShootVigor().
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
	const float apicalControl = rootGrowthParameters.m_apicalControl;

	const float rootMaintenanceVigor = glm::min(m_rootSkeleton.m_data.m_vigor, 0.0f);
	float rootMaintenanceVigorFillingRate = 0.0f;
	const float rootDevelopmentVigor = m_rootSkeleton.m_data.m_vigor - rootMaintenanceVigor;
	if (m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor != 0.0f) {
		m_rootNodeDevelopmentRate = rootDevelopmentVigor / m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor;
	}

	for (const auto& rootNodeHandle : sortedRootNodeList) {
		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& rootNodeData = rootNode.m_data;
		auto& rootNodeVigorFlow = rootNodeData.m_vigorFlow;

		rootNodeData.m_vigorSink.AddVigor(rootMaintenanceVigorFillingRate * rootNodeData.m_vigorSink.GetMaintenanceVigorRequirement());

		if (rootNode.GetParentHandle() == -1) {
			if (m_rootSkeleton.m_data.m_vigorRequirement.m_nodeDevelopmentalVigor != 0.0f) {
				const float totalRequirement = rootNodeVigorFlow.m_vigorRequirementWeight + rootNodeVigorFlow.m_subtreeVigorRequirementWeight;
				if (totalRequirement != 0.0f) {
					rootNodeVigorFlow.m_allocatedVigor = rootDevelopmentVigor * rootNodeVigorFlow.m_vigorRequirementWeight / totalRequirement;
				}
				rootNodeData.m_vigorSink.AddVigor(rootNodeVigorFlow.m_allocatedVigor);
				rootNodeVigorFlow.m_subTreeAllocatedVigor = rootDevelopmentVigor - rootNodeVigorFlow.m_allocatedVigor;
			}
		}
		rootNodeData.m_vigorSink.AddVigor(rootNodeVigorFlow.m_allocatedVigor);

		if (rootNodeVigorFlow.m_subTreeAllocatedVigor != 0.0f) {
			float childDevelopmentalVigorRequirementWeightSum = 0.0f;
			for (const auto& i : rootNode.RefChildHandles()) {
				const auto& childRootNode = m_rootSkeleton.RefNode(i);
				const auto& childRootNodeData = childRootNode.m_data;
				const auto& childRootNodeVigorFlow = childRootNodeData.m_vigorFlow;
				float childDevelopmentalVigorRequirementWeight = 0.0f;
				if (rootNodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f) {
					childDevelopmentalVigorRequirementWeight =
						(childRootNodeVigorFlow.m_vigorRequirementWeight + childRootNodeVigorFlow.m_subtreeVigorRequirementWeight)
						/ rootNodeVigorFlow.m_subtreeVigorRequirementWeight;
				}
				//Perform Apical control here.
				if (rootNodeData.m_isMaxChild) childDevelopmentalVigorRequirementWeight *= apicalControl;
				childDevelopmentalVigorRequirementWeightSum += childDevelopmentalVigorRequirementWeight;
			}
			for (const auto& i : rootNode.RefChildHandles()) {
				auto& childRootNode = m_rootSkeleton.RefNode(i);
				auto& childRootNodeData = childRootNode.m_data;
				auto& childRootNodeVigorFlow = childRootNodeData.m_vigorFlow;
				float childDevelopmentalVigorRequirementWeight = 0.0f;
				if (rootNodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f)
					childDevelopmentalVigorRequirementWeight = (childRootNodeVigorFlow.m_vigorRequirementWeight + childRootNodeVigorFlow.m_subtreeVigorRequirementWeight)
					/ rootNodeVigorFlow.m_subtreeVigorRequirementWeight;

				//Perform Apical control here.
				if (rootNodeData.m_isMaxChild) childDevelopmentalVigorRequirementWeight *= apicalControl;

				//Then calculate total amount of development vigor belongs to this child from internode received vigor for its children.
				float childTotalAllocatedDevelopmentVigor = 0.0f;
				if (childDevelopmentalVigorRequirementWeightSum != 0.0f) childTotalAllocatedDevelopmentVigor = rootNodeVigorFlow.m_subTreeAllocatedVigor *
					childDevelopmentalVigorRequirementWeight / childDevelopmentalVigorRequirementWeightSum;

				childRootNodeVigorFlow.m_allocatedVigor = 0.0f;
				if (childRootNodeVigorFlow.m_vigorRequirementWeight + childRootNodeVigorFlow.m_subtreeVigorRequirementWeight != 0.0f) {
					childRootNodeVigorFlow.m_allocatedVigor += childTotalAllocatedDevelopmentVigor *
						childRootNodeVigorFlow.m_vigorRequirementWeight
						/ (childRootNodeVigorFlow.m_vigorRequirementWeight + childRootNodeVigorFlow.m_subtreeVigorRequirementWeight);
				}
				childRootNodeVigorFlow.m_subTreeAllocatedVigor = childTotalAllocatedDevelopmentVigor - childRootNodeVigorFlow.m_allocatedVigor;
			}
		}
		else
		{
			for (const auto& i : rootNode.RefChildHandles())
			{
				auto& childRootNode = m_rootSkeleton.RefNode(i);
				auto& childRootNodeData = childRootNode.m_data;
				auto& childRootNodeVigorFlow = childRootNodeData.m_vigorFlow;
				childRootNodeVigorFlow.m_allocatedVigor = childRootNodeVigorFlow.m_subTreeAllocatedVigor = 0.0f;
			}
		}
	}
}

void TreeModel::CalculateVigorRequirement(const RootGrowthController& rootGrowthParameters,
	RootGrowthRequirement& newRootGrowthNutrientsRequirement)
{
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
	for (const auto& rootNodeHandle : sortedRootNodeList) {
		auto& rootNode = m_rootSkeleton.RefNode(rootNodeHandle);
		auto& rootNodeData = rootNode.m_data;
		auto& rootNodeVigorFlow = rootNodeData.m_vigorFlow;
		//This one has 0 always but we will put value in it in future.
		rootNodeVigorFlow.m_vigorRequirementWeight = 0.0f;
		float growthPotential = 0.0f;
		if (rootNode.RefChildHandles().empty())
		{
			growthPotential = m_currentDeltaTime * rootGrowthParameters.m_rootNodeGrowthRate * rootGrowthParameters.m_rootNodeVigorRequirement;
			rootNodeVigorFlow.m_vigorRequirementWeight =
				rootNodeData.m_nitrite * growthPotential * (1.0f - rootGrowthParameters.m_environmentalFriction(rootNode));
		}
		rootNodeData.m_vigorSink.SetDesiredMaintenanceVigorRequirement(0.0f);
		rootNodeData.m_vigorSink.SetDesiredDevelopmentalVigorRequirement(rootNodeVigorFlow.m_vigorRequirementWeight);
		//We sum the vigor requirement with the developmentalVigorRequirement,
		//so the overall nitrite will not affect the root growth. Thus we will have same growth rate for low/high nitrite density.
		newRootGrowthNutrientsRequirement.m_nodeDevelopmentalVigor += growthPotential;
	}
}

void TreeModel::SampleTemperature(const glm::mat4& globalTransform, ClimateModel& climateModel)
{
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	for (auto it = sortedInternodeList.rbegin(); it != sortedInternodeList.rend(); ++it) {
		auto& internode = m_shootSkeleton.RefNode(*it);
		auto& internodeData = internode.m_data;
		auto& internodeInfo = internode.m_info;
		internodeData.m_temperature = climateModel.GetTemperature(globalTransform * glm::translate(internodeInfo.m_globalPosition)[3]);
	}
}

void TreeModel::SampleSoilDensity(const glm::mat4& globalTransform, VoxelSoilModel& soilModel)
{
	const auto& sortedRootNodeList = m_rootSkeleton.RefSortedNodeList();
	for (auto it = sortedRootNodeList.rbegin(); it != sortedRootNodeList.rend(); ++it) {
		auto& rootNode = m_rootSkeleton.RefNode(*it);
		auto& rootNodeData = rootNode.m_data;
		auto& rootNodeInfo = rootNode.m_info;
		rootNodeData.m_soilDensity = soilModel.GetDensity(globalTransform * glm::translate(rootNodeInfo.m_globalPosition)[3]);
	}
}

ShootSkeleton& TreeModel::RefShootSkeleton() {
	return m_shootSkeleton;
}

const ShootSkeleton&
TreeModel::PeekShootSkeleton(const int iteration) const {
	assert(iteration < 0 || iteration <= m_history.size());
	if (iteration == m_history.size() || iteration < 0) return m_shootSkeleton;
	return m_history.at(iteration).first;
}

RootSkeleton& TreeModel::RefRootSkeleton() {
	return m_rootSkeleton;
}

const RootSkeleton&
TreeModel::PeekRootSkeleton(const int iteration) const {
	assert(iteration < 0 || iteration <= m_history.size());
	if (iteration == m_history.size() || iteration < 0) return m_rootSkeleton;
	return m_history.at(iteration).second;
}

void TreeModel::ClearHistory() {
	m_history.clear();
}

void TreeModel::Step() {
	m_history.emplace_back(m_shootSkeleton, m_rootSkeleton);
	if (m_historyLimit > 0) {
		while (m_history.size() > m_historyLimit) {
			m_history.pop_front();
		}
	}
}

void TreeModel::Pop() {
	m_history.pop_back();
}

int TreeModel::CurrentIteration() const {
	return m_history.size();
}

void TreeModel::Reverse(int iteration) {
	assert(iteration >= 0 && iteration < m_history.size());
	m_shootSkeleton = m_history[iteration].first;
	m_rootSkeleton = m_history[iteration].second;
	m_history.erase((m_history.begin() + iteration), m_history.end());
}

void TreeModel::ExportTreeIOSkeleton(treeio::ArrayTree& arrayTree) const
{
	using namespace treeio;
	const auto& sortedInternodeList = m_shootSkeleton.RefSortedNodeList();
	if (sortedInternodeList.empty()) return;
	const auto& rootNode = m_shootSkeleton.PeekNode(0);
	TreeNodeData rootNodeData;
	//rootNodeData.direction = rootNode.m_info.m_regulatedGlobalRotation * glm::vec3(0, 0, -1);
	rootNodeData.thickness = rootNode.m_info.m_thickness;
	rootNodeData.pos = rootNode.m_info.m_globalPosition;

	auto rootId = arrayTree.addRoot(rootNodeData);
	std::unordered_map<NodeHandle, size_t> nodeMap;
	nodeMap[0] = rootId;
	for (const auto& nodeHandle : sortedInternodeList)
	{
		if (nodeHandle == 0) continue;
		const auto& node = m_shootSkeleton.PeekNode(nodeHandle);
		TreeNodeData nodeData;
		//nodeData.direction = node.m_info.m_regulatedGlobalRotation * glm::vec3(0, 0, -1);
		nodeData.thickness = node.m_info.m_thickness;
		nodeData.pos = node.m_info.m_globalPosition;

		auto currentId = arrayTree.addNodeChild(nodeMap[node.GetParentHandle()], nodeData);
		nodeMap[nodeHandle] = currentId;
	}
}

void RootGrowthController::SetTropisms(Node<RootNodeGrowthData>& oldNode, Node<RootNodeGrowthData>& newNode) const
{
	const float probability = m_tropismSwitchingProbability *
		glm::exp(-m_tropismSwitchingProbabilityDistanceFactor * oldNode.m_data.m_rootDistance);

	const bool needSwitch = probability >= glm::linearRand(0.0f, 1.0f);
	newNode.m_data.m_horizontalTropism = needSwitch ? oldNode.m_data.m_verticalTropism : oldNode.m_data.m_horizontalTropism;
	newNode.m_data.m_verticalTropism = needSwitch ? oldNode.m_data.m_horizontalTropism : oldNode.m_data.m_verticalTropism;
}
#pragma endregion
