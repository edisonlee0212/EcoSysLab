#include "ShootDescriptor.hpp"

using namespace EcoSysLab;

void ShootDescriptor::PrepareController(ShootGrowthController& shootGrowthController)
{
	shootGrowthController.m_baseInternodeCount = m_baseInternodeCount;

	shootGrowthController.m_baseNodeApicalAngle = [&](const Node<InternodeGrowthData>& internode)
		{
			return glm::gaussRand(m_baseNodeApicalAngleMeanVariance.x, m_baseNodeApicalAngleMeanVariance.y);
		};

	shootGrowthController.m_internodeGrowthRate = m_growthRate / m_internodeLength;

	shootGrowthController.m_branchingAngle = [&](const Node<InternodeGrowthData>& internode)
		{
			float value = glm::gaussRand(m_branchingAngleMeanVariance.x, m_branchingAngleMeanVariance.y);
			if(const auto noise = m_branchingAngleNoise.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}
			return value;
		};
	shootGrowthController.m_rollAngle = [&](const Node<InternodeGrowthData>& internode)
		{
			float value = glm::gaussRand(m_rollAngleMeanVariance.x, m_rollAngleMeanVariance.y);
			if (const auto noise = m_rollAngleNoise.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}
			return value;
		};
	shootGrowthController.m_apicalAngle = [&](const Node<InternodeGrowthData>& internode)
		{
			float value = glm::gaussRand(m_apicalAngleMeanVariance.x, m_apicalAngleMeanVariance.y);
			if (const auto noise = m_apicalAngleNoise.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}
			return value;
		};
	shootGrowthController.m_gravitropism = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_gravitropism;
		};
	shootGrowthController.m_phototropism = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_phototropism;
		};
	shootGrowthController.m_horizontalTropism = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_horizontalTropism;
		};
	shootGrowthController.m_sagging = [&](const Node<InternodeGrowthData>& internode)
		{
			const auto newSagging = glm::min(
				m_saggingFactorThicknessReductionMax.z,
				m_saggingFactorThicknessReductionMax.x *
				(internode.m_data.m_descendantTotalBiomass + internode.m_data.m_extraMass) /
				glm::pow(
					internode.m_info.m_thickness /
					m_endNodeThickness,
					m_saggingFactorThicknessReductionMax.y));
			return glm::max(internode.m_data.m_sagging, newSagging);
		};
	shootGrowthController.m_internodeLength = m_internodeLength;
	shootGrowthController.m_internodeLengthThicknessFactor = m_internodeLengthThicknessFactor;
	shootGrowthController.m_endNodeThickness = m_endNodeThickness;
	shootGrowthController.m_thicknessAccumulationFactor = m_thicknessAccumulationFactor;
	shootGrowthController.m_thicknessAgeFactor = m_thicknessAgeFactor;
	shootGrowthController.m_internodeShadowFactor = m_internodeShadowFactor;

	shootGrowthController.m_lateralBudCount = m_lateralBudCount;
	shootGrowthController.m_apicalBudExtinctionRate = [&](const Node<InternodeGrowthData>& internode)
		{
			if (internode.m_info.m_rootDistance < 0.5f) return 0.f;
			return m_apicalBudExtinctionRate;
		};
	shootGrowthController.m_lateralBudFlushingRate = [&](const Node<InternodeGrowthData>& internode)
		{
			float flushingRate = m_lateralBudFlushingRate * internode.m_data.m_lightIntensity;
			if (internode.m_data.m_inhibitorSink > 0.0f) flushingRate *= glm::exp(-internode.m_data.m_inhibitorSink);
			return flushingRate;
		};
	shootGrowthController.m_apicalControl = m_apicalControl;
	shootGrowthController.m_apicalDominance = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_apicalDominance * internode.m_data.m_lightIntensity;
		};
	shootGrowthController.m_apicalDominanceLoss = m_apicalDominanceLoss;

	shootGrowthController.m_lowBranchPruning = m_lowBranchPruning;
	shootGrowthController.m_lowBranchPruningThicknessFactor = m_lowBranchPruningThicknessFactor;
	shootGrowthController.m_pruningFactor = [&](const ShootSkeleton& shootSkeleton, const Node<InternodeGrowthData>& internode)
		{
			if (m_trunkProtection && internode.m_data.m_order == 0)
			{
				return 0.f;
			}
			float pruningProbability = 0.0f;
			if (m_maxFlowLength != 0 && m_maxFlowLength < internode.m_info.m_chainIndex)
			{
				pruningProbability += 999.f;
			}
			if (internode.IsEndNode()) {
				if (internode.m_data.m_lightIntensity <= m_lightPruningFactor)
				{
					pruningProbability += m_lightPruningProbability;
				}
			}
			if (internode.m_data.m_level != 0 && m_thicknessPruningFactor != 0.0f
				&& internode.m_info.m_thickness / internode.m_info.m_endDistance < m_thicknessPruningFactor)
			{
				pruningProbability += m_thicknessPruningProbability;
			}
			return pruningProbability;
		};


	shootGrowthController.m_leafGrowthRate = m_leafGrowthRate;
	shootGrowthController.m_fruitGrowthRate = m_fruitGrowthRate;

	shootGrowthController.m_fruitBudCount = m_fruitBudCount;
	shootGrowthController.m_leafBudCount = m_leafBudCount;

	shootGrowthController.m_leafBudFlushingProbability = [&](const Node<InternodeGrowthData>& internode)
		{
			const auto& internodeData = internode.m_data;
			const auto& probabilityRange = m_leafBudFlushingProbabilityTemperatureRange;
			float flushProbability = glm::mix(probabilityRange.x, probabilityRange.y,
				glm::clamp((internodeData.m_temperature - probabilityRange.z) / (probabilityRange.w - probabilityRange.z), 0.0f, 1.0f));
			flushProbability *= internodeData.m_lightIntensity;
			return flushProbability;
		};
	shootGrowthController.m_fruitBudFlushingProbability = [&](const Node<InternodeGrowthData>& internode)
		{
			const auto& internodeData = internode.m_data;
			const auto& probabilityRange = m_fruitBudFlushingProbabilityTemperatureRange;
			float flushProbability = glm::mix(probabilityRange.x, probabilityRange.y,
				glm::clamp((internodeData.m_temperature - probabilityRange.z) / (probabilityRange.w - probabilityRange.z), 0.0f, 1.0f));
			flushProbability *= internodeData.m_lightIntensity;
			return flushProbability;
		};

	shootGrowthController.m_leafVigorRequirement = m_leafVigorRequirement;
	shootGrowthController.m_fruitVigorRequirement = m_fruitVigorRequirement;



	shootGrowthController.m_maxLeafSize = m_maxLeafSize;
	shootGrowthController.m_leafPositionVariance = m_leafPositionVariance;
	shootGrowthController.m_leafRotationVariance = m_leafRotationVariance;
	
	shootGrowthController.m_leafFallProbability = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_leafFallProbability;
		};
	shootGrowthController.m_maxFruitSize = m_maxFruitSize;
	shootGrowthController.m_fruitPositionVariance = m_fruitPositionVariance;
	shootGrowthController.m_fruitRotationVariance = m_fruitRotationVariance;
	shootGrowthController.m_fruitDamage = [=](const Node<InternodeGrowthData>& internode)
		{
			float fruitDamage = 0.0f;
			return fruitDamage;
		};
	shootGrowthController.m_fruitFallProbability = [&](const Node<InternodeGrowthData>& internode)
		{
			return m_fruitFallProbability;
		};

}

void ShootDescriptor::Serialize(YAML::Emitter& out)
{
	out << YAML::Key << "m_baseInternodeCount" << YAML::Value << m_baseInternodeCount;
	out << YAML::Key << "m_baseNodeApicalAngleMeanVariance" << YAML::Value << m_baseNodeApicalAngleMeanVariance;

	out << YAML::Key << "m_growthRate" << YAML::Value << m_growthRate;
	m_branchingAngleNoise.Save("m_branchingAngleNoise", out);
	m_rollAngleNoise.Save("m_rollAngleNoise", out);
	m_apicalAngleNoise.Save("m_apicalAngleNoise", out);
	out << YAML::Key << "m_branchingAngleMeanVariance" << YAML::Value << m_branchingAngleMeanVariance;
	out << YAML::Key << "m_rollAngleMeanVariance" << YAML::Value << m_rollAngleMeanVariance;
	out << YAML::Key << "m_apicalAngleMeanVariance" << YAML::Value << m_apicalAngleMeanVariance;
	out << YAML::Key << "m_gravitropism" << YAML::Value << m_gravitropism;
	out << YAML::Key << "m_phototropism" << YAML::Value << m_phototropism;
	out << YAML::Key << "m_horizontalTropism" << YAML::Value << m_horizontalTropism;
	out << YAML::Key << "m_saggingFactorThicknessReductionMax" << YAML::Value << m_saggingFactorThicknessReductionMax;
	out << YAML::Key << "m_internodeLength" << YAML::Value << m_internodeLength;
	out << YAML::Key << "m_internodeLengthThicknessFactor" << YAML::Value << m_internodeLengthThicknessFactor;
	out << YAML::Key << "m_endNodeThickness" << YAML::Value << m_endNodeThickness;
	out << YAML::Key << "m_thicknessAccumulationFactor" << YAML::Value << m_thicknessAccumulationFactor;
	out << YAML::Key << "m_thicknessAgeFactor" << YAML::Value << m_thicknessAgeFactor;
	out << YAML::Key << "m_internodeShadowFactor" << YAML::Value << m_internodeShadowFactor;

	out << YAML::Key << "m_lateralBudCount" << YAML::Value << m_lateralBudCount;
	out << YAML::Key << "m_apicalBudExtinctionRate" << YAML::Value << m_apicalBudExtinctionRate;
	out << YAML::Key << "m_lateralBudFlushingRate" << YAML::Value << m_lateralBudFlushingRate;
	out << YAML::Key << "m_apicalControl" << YAML::Value << m_apicalControl;
	out << YAML::Key << "m_apicalDominance" << YAML::Value << m_apicalDominance;
	out << YAML::Key << "m_apicalDominanceLoss" << YAML::Value << m_apicalDominanceLoss;

	out << YAML::Key << "m_trunkProtection" << YAML::Value << m_trunkProtection;
	out << YAML::Key << "m_maxFlowLength" << YAML::Value << m_maxFlowLength;
	out << YAML::Key << "m_lowBranchPruning" << YAML::Value << m_lowBranchPruning;
	out << YAML::Key << "m_lowBranchPruningThicknessFactor" << YAML::Value << m_lowBranchPruningThicknessFactor;
	out << YAML::Key << "m_lightPruningFactor" << YAML::Value << m_lightPruningFactor;
	out << YAML::Key << "m_thicknessPruningFactor" << YAML::Value << m_thicknessPruningFactor;

	out << YAML::Key << "m_lightPruningProbability" << YAML::Value << m_lightPruningProbability;
	out << YAML::Key << "m_thicknessPruningProbability" << YAML::Value << m_thicknessPruningProbability;

	out << YAML::Key << "m_leafBudCount" << YAML::Value << m_leafBudCount;
	out << YAML::Key << "m_leafGrowthRate" << YAML::Value << m_leafGrowthRate;
	out << YAML::Key << "m_leafBudFlushingProbabilityTemperatureRange" << YAML::Value << m_leafBudFlushingProbabilityTemperatureRange;
	out << YAML::Key << "m_leafVigorRequirement" << YAML::Value << m_leafVigorRequirement;
	out << YAML::Key << "m_maxLeafSize" << YAML::Value << m_maxLeafSize;
	out << YAML::Key << "m_leafPositionVariance" << YAML::Value << m_leafPositionVariance;
	out << YAML::Key << "m_leafRotationVariance" << YAML::Value << m_leafRotationVariance;
	out << YAML::Key << "m_leafChlorophyllLoss" << YAML::Value << m_leafChlorophyllLoss;
	out << YAML::Key << "m_leafChlorophyllSynthesisFactorTemperature" << YAML::Value << m_leafChlorophyllSynthesisFactorTemperature;
	out << YAML::Key << "m_leafFallProbability" << YAML::Value << m_leafFallProbability;
	out << YAML::Key << "m_leafDistanceToBranchEndLimit" << YAML::Value << m_leafDistanceToBranchEndLimit;

	out << YAML::Key << "m_fruitBudCount" << YAML::Value << m_fruitBudCount;
	out << YAML::Key << "m_fruitGrowthRate" << YAML::Value << m_fruitGrowthRate;
	out << YAML::Key << "m_fruitBudFlushingProbabilityTemperatureRange" << YAML::Value << m_fruitBudFlushingProbabilityTemperatureRange;
	out << YAML::Key << "m_fruitVigorRequirement" << YAML::Value << m_fruitVigorRequirement;
	out << YAML::Key << "m_maxFruitSize" << YAML::Value << m_maxFruitSize;
	out << YAML::Key << "m_fruitPositionVariance" << YAML::Value << m_fruitPositionVariance;
	out << YAML::Key << "m_fruitRotationVariance" << YAML::Value << m_fruitRotationVariance;
	out << YAML::Key << "m_fruitFallProbability" << YAML::Value << m_fruitFallProbability;

	m_barkMaterial.Save("m_barkMaterial", out);
}

void ShootDescriptor::Deserialize(const YAML::Node& in)
{
	if (in["m_baseInternodeCount"]) m_baseInternodeCount = in["m_baseInternodeCount"].as<float>();
	if (in["m_baseNodeApicalAngleMeanVariance"]) m_baseNodeApicalAngleMeanVariance = in["m_baseNodeApicalAngleMeanVariance"].as<glm::vec2>();

	if (in["m_growthRate"]) m_growthRate = in["m_growthRate"].as<float>();
	m_branchingAngleNoise.Load("m_branchingAngleNoise", in);
	m_rollAngleNoise.Load("m_rollAngleNoise", in);
	m_apicalAngleNoise.Load("m_apicalAngleNoise", in);
	if (in["m_branchingAngleMeanVariance"]) m_branchingAngleMeanVariance = in["m_branchingAngleMeanVariance"].as<glm::vec2>();
	if (in["m_rollAngleMeanVariance"]) m_rollAngleMeanVariance = in["m_rollAngleMeanVariance"].as<glm::vec2>();
	if (in["m_apicalAngleMeanVariance"]) m_apicalAngleMeanVariance = in["m_apicalAngleMeanVariance"].as<glm::vec2>();
	if (in["m_gravitropism"]) m_gravitropism = in["m_gravitropism"].as<float>();
	if (in["m_phototropism"]) m_phototropism = in["m_phototropism"].as<float>();
	if (in["m_horizontalTropism"]) m_horizontalTropism = in["m_horizontalTropism"].as<float>();
	if (in["m_saggingFactorThicknessReductionMax"]) m_saggingFactorThicknessReductionMax = in["m_saggingFactorThicknessReductionMax"].as<glm::vec3>();
	if (in["m_internodeLength"]) m_internodeLength = in["m_internodeLength"].as<float>();
	if (in["m_internodeLengthThicknessFactor"]) m_internodeLengthThicknessFactor = in["m_internodeLengthThicknessFactor"].as<float>();
	if (in["m_endNodeThickness"]) m_endNodeThickness = in["m_endNodeThickness"].as<float>();
	if (in["m_thicknessAccumulationFactor"]) m_thicknessAccumulationFactor = in["m_thicknessAccumulationFactor"].as<float>();
	if (in["m_thicknessAgeFactor"]) m_thicknessAgeFactor = in["m_thicknessAgeFactor"].as<float>();
	if (in["m_internodeShadowFactor"]) m_internodeShadowFactor = in["m_internodeShadowFactor"].as<float>();


	if (in["m_lateralBudCount"]) m_lateralBudCount = in["m_lateralBudCount"].as<int>();
	if (in["m_apicalBudExtinctionRate"]) m_apicalBudExtinctionRate = in["m_apicalBudExtinctionRate"].as<float>();
	if (in["m_lateralBudFlushingRate"]) m_lateralBudFlushingRate = in["m_lateralBudFlushingRate"].as<float>();
	if (in["m_apicalControl"]) m_apicalControl = in["m_apicalControl"].as<float>();
	if (in["m_apicalDominance"]) m_apicalDominance = in["m_apicalDominance"].as<float>();
	if (in["m_apicalDominanceLoss"]) m_apicalDominanceLoss = in["m_apicalDominanceLoss"].as<float>();

	if (in["m_trunkProtection"]) m_trunkProtection = in["m_trunkProtection"].as<bool>();
	if (in["m_maxFlowLength"]) m_maxFlowLength = in["m_maxFlowLength"].as<int>();
	if (in["m_lowBranchPruning"]) m_lowBranchPruning = in["m_lowBranchPruning"].as<float>();
	if (in["m_lowBranchPruningThicknessFactor"]) m_lowBranchPruningThicknessFactor = in["m_lowBranchPruningThicknessFactor"].as<float>();
	if (in["m_lightPruningFactor"]) m_lightPruningFactor = in["m_lightPruningFactor"].as<float>();
	if (in["m_thicknessPruningFactor"]) m_thicknessPruningFactor = in["m_thicknessPruningFactor"].as<float>();
	if (in["m_lightPruningProbability"]) m_lightPruningProbability = in["m_lightPruningProbability"].as<float>();
	if (in["m_thicknessPruningProbability"]) m_thicknessPruningProbability = in["m_thicknessPruningProbability"].as<float>();


	if (in["m_leafBudCount"]) m_leafBudCount = in["m_leafBudCount"].as<int>();
	if (in["m_leafGrowthRate"]) m_leafGrowthRate = in["m_leafGrowthRate"].as<float>();
	if (in["m_leafBudFlushingProbabilityTemperatureRange"]) m_leafBudFlushingProbabilityTemperatureRange = in["m_leafBudFlushingProbabilityTemperatureRange"].as< glm::vec4>();
	if (in["m_leafVigorRequirement"]) m_leafVigorRequirement = in["m_leafVigorRequirement"].as<float>();
	if (in["m_maxLeafSize"]) m_maxLeafSize = in["m_maxLeafSize"].as<glm::vec3>();
	if (in["m_leafPositionVariance"]) m_leafPositionVariance = in["m_leafPositionVariance"].as<float>();
	if (in["m_leafRotationVariance"]) m_leafRotationVariance = in["m_leafRotationVariance"].as<float>();
	if (in["m_leafChlorophyllLoss"]) m_leafChlorophyllLoss = in["m_leafChlorophyllLoss"].as<float>();
	if (in["m_leafChlorophyllSynthesisFactorTemperature"]) m_leafChlorophyllSynthesisFactorTemperature = in["m_leafChlorophyllSynthesisFactorTemperature"].as<float>();
	if (in["m_leafFallProbability"]) m_leafFallProbability = in["m_leafFallProbability"].as<float>();
	if (in["m_leafDistanceToBranchEndLimit"]) m_leafDistanceToBranchEndLimit = in["m_leafDistanceToBranchEndLimit"].as<float>();

	//Structure
	if (in["m_fruitBudCount"]) m_fruitBudCount = in["m_fruitBudCount"].as<int>();
	if (in["m_fruitGrowthRate"]) m_fruitGrowthRate = in["m_fruitGrowthRate"].as<float>();
	if (in["m_fruitBudFlushingProbabilityTemperatureRange"]) m_fruitBudFlushingProbabilityTemperatureRange = in["m_fruitBudFlushingProbabilityTemperatureRange"].as<glm::vec4>();
	if (in["m_fruitVigorRequirement"]) m_fruitVigorRequirement = in["m_fruitVigorRequirement"].as<float>();
	if (in["m_maxFruitSize"]) m_maxFruitSize = in["m_maxFruitSize"].as<glm::vec3>();
	if (in["m_fruitPositionVariance"]) m_fruitPositionVariance = in["m_fruitPositionVariance"].as<float>();
	if (in["m_fruitRotationVariance"]) m_fruitRotationVariance = in["m_fruitRotationVariance"].as<float>();
	if (in["m_fruitFallProbability"]) m_fruitFallProbability = in["m_fruitFallProbability"].as<float>();

	m_barkMaterial.Load("m_barkMaterial", in);
}

void ShootDescriptor::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer)
{
	bool changed = false;
	changed = ImGui::DragFloat("Growth rate", &m_growthRate, 0.01f, 0.0f, 10.0f) || changed;
	if (ImGui::TreeNodeEx("Internode", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed = ImGui::DragInt("Base node count", &m_baseInternodeCount, 1, 0, 3) || changed;
		changed = ImGui::DragFloat2("Base apical angle mean/var", &m_baseNodeApicalAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;

		changed = ImGui::DragInt("Lateral bud count", &m_lateralBudCount, 1, 0, 3) || changed;
		changed = ImGui::DragFloat2("Branching angle base/var", &m_branchingAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
		editorLayer->DragAndDropButton<ProceduralNoise2D>(m_branchingAngleNoise, "Branching Angle Noise");
		changed = ImGui::DragFloat2("Roll angle base/var", &m_rollAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
		editorLayer->DragAndDropButton<ProceduralNoise2D>(m_rollAngleNoise, "Roll Angle Noise");
		changed = ImGui::DragFloat2("Apical angle base/var", &m_apicalAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
		editorLayer->DragAndDropButton<ProceduralNoise2D>(m_apicalAngleNoise, "Apical Angle Noise");
		changed = ImGui::DragFloat("Internode length", &m_internodeLength, 0.001f) || changed;
		changed = ImGui::DragFloat("Internode length thickness factor", &m_internodeLengthThicknessFactor, 0.0001f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat3("Thickness min/factor/age", &m_endNodeThickness, 0.0001f, 0.0f, 1.0f, "%.6f") || changed;

		changed = ImGui::DragFloat("Sagging strength", &m_saggingFactorThicknessReductionMax.x, 0.0001f, 0.0f, 10.0f, "%.5f") || changed;
		changed = ImGui::DragFloat("Sagging thickness factor", &m_saggingFactorThicknessReductionMax.y, 0.01f, 0.0f, 10.0f, "%.5f") || changed;
		changed = ImGui::DragFloat("Sagging max", &m_saggingFactorThicknessReductionMax.z, 0.001f, 0.0f, 1.0f, "%.5f") || changed;


		changed = ImGui::DragFloat("Internode shadow factor", &m_internodeShadowFactor, 0.001f, 0.0f, 1.0f) || changed;
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Bud fate", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::DragFloat("Gravitropism", &m_gravitropism, 0.01f) || changed;
		changed = ImGui::DragFloat("Phototropism", &m_phototropism, 0.01f) || changed;
		changed = ImGui::DragFloat("Horizontal Tropism", &m_horizontalTropism, 0.01f) || changed;

		changed = ImGui::DragFloat("Apical bud extinction rate", &m_apicalBudExtinctionRate, 0.00001f, 0.0f, 1.0f, "%.5f") || changed;
		changed = ImGui::DragFloat("Lateral bud flushing rate", &m_lateralBudFlushingRate, 0.00001f, 0.0f, 1.0f, "%.5f") || changed;
		changed = ImGui::DragFloat("Apical control", &m_apicalControl, 0.01f) || changed;
		changed = ImGui::DragFloat2("Inhibitor val/loss", &m_apicalDominance, 0.01f) || changed;
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Pruning", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed = ImGui::Checkbox("Trunk Protection", &m_trunkProtection) || changed;
		changed = ImGui::DragInt("Max chain length", &m_maxFlowLength, 1) || changed;
		changed = ImGui::DragFloat("Low branch pruning", &m_lowBranchPruning, 0.01f) || changed;

		if (m_lowBranchPruning > 0.0f) changed = ImGui::DragFloat("Low branch pruning thickness factor", &m_lowBranchPruningThicknessFactor, 0.01f) || changed;

		changed = ImGui::DragFloat("Light pruning", &m_lightPruningFactor, 0.01f) || changed;
		changed = ImGui::DragFloat("Light pruning prob", &m_lightPruningProbability, 0.01f) || changed;

		changed = ImGui::DragFloat("Thin branch pruning", &m_thicknessPruningFactor, 0.01f, 0.0f) || changed;
		changed = ImGui::DragFloat("Thin branch pruning prob", &m_thicknessPruningProbability, 0.01f, 0.0f) || changed;
		ImGui::TreePop();
	}
	changed = ImGui::DragInt("Leaf bud count", &m_leafBudCount, 1, 0, 3) || changed;
	if (m_leafBudCount > 0 && ImGui::TreeNodeEx("Leaf"))
	{
		changed = ImGui::DragFloat("Leaf growth rate", &m_leafGrowthRate, 0.01f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat4("Leaf flushing prob/temp range", &m_leafBudFlushingProbabilityTemperatureRange.x, 0.00001f, 0.0f, 1.0f, "%.5f") || changed;
		changed = ImGui::DragFloat3("Leaf vigor requirement", &m_leafVigorRequirement, 0.01f) || changed;
		changed = ImGui::DragFloat3("Size", &m_maxLeafSize.x, 0.01f) || changed;
		changed = ImGui::DragFloat("Position Variance", &m_leafPositionVariance, 0.01f) || changed;
		changed = ImGui::DragFloat("Random rotation", &m_leafRotationVariance, 0.01f) || changed;
		changed = ImGui::DragFloat("Chlorophyll Loss", &m_leafChlorophyllLoss, 0.01f) || changed;
		changed = ImGui::DragFloat("Chlorophyll temperature", &m_leafChlorophyllSynthesisFactorTemperature, 0.01f) || changed;
		changed = ImGui::DragFloat("Drop prob", &m_leafFallProbability, 0.01f) || changed;
		changed = ImGui::DragFloat("Distance To End Limit", &m_leafDistanceToBranchEndLimit, 0.01f) || changed;
		ImGui::TreePop();
	}
	changed = ImGui::DragInt("Fruit bud count", &m_fruitBudCount, 1, 0, 3) || changed;
	if (m_fruitBudCount > 0 && ImGui::TreeNodeEx("Fruit"))
	{
		changed = ImGui::DragFloat("Fruit growth rate", &m_fruitGrowthRate, 0.01f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat4("Fruit flushing prob/temp range", &m_fruitBudFlushingProbabilityTemperatureRange.x, 0.00001f, 0.0f, 1.0f, "%.5f") || changed;
		changed = ImGui::DragFloat3("Fruit vigor requirement", &m_fruitVigorRequirement, 0.01f) || changed;
		changed = ImGui::DragFloat3("Size", &m_maxFruitSize.x, 0.01f) || changed;
		changed = ImGui::DragFloat("Position Variance", &m_fruitPositionVariance, 0.01f) || changed;
		changed = ImGui::DragFloat("Random rotation", &m_fruitRotationVariance, 0.01f) || changed;

		changed = ImGui::DragFloat("Drop prob", &m_fruitFallProbability, 0.01f) || changed;
		ImGui::TreePop();
	}

	editorLayer->DragAndDropButton<Material>(m_barkMaterial, "Bark Material##SBS");

	if (changed) m_saved = false;
}

void ShootDescriptor::CollectAssetRef(std::vector<AssetRef>& list)
{
	if (m_branchingAngleNoise.Get<ProceduralNoise2D>()) list.push_back(m_branchingAngleNoise);
	if (m_rollAngleNoise.Get<ProceduralNoise2D>()) list.push_back(m_rollAngleNoise);
	if (m_apicalAngleNoise.Get<ProceduralNoise2D>()) list.push_back(m_apicalAngleNoise);

	if (m_barkMaterial.Get<Material>()) list.push_back(m_barkMaterial);
}
