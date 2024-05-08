#include "ShootDescriptor.hpp"

using namespace EcoSysLab;

void ShootDescriptor::PrepareController(ShootGrowthController& shootGrowthController) const
{
	shootGrowthController.m_baseInternodeCount = m_baseInternodeCount;
	shootGrowthController.m_breakingForce = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			if (m_branchStrength != 0.f && !internode.IsEndNode() && internode.m_info.m_thickness != 0.f && internode.m_info.m_length != 0.f) {
				float branchWaterFactor = 1.f;
				if (m_branchStrengthLightingThreshold != 0.f && internode.m_data.m_maxDescendantLightIntensity < m_branchStrengthLightingThreshold)
				{
					branchWaterFactor = 1.f - m_branchStrengthLightingLoss;
				}

				return glm::pow(internode.m_info.m_thickness / m_endNodeThickness, m_branchStrengthThicknessFactor) * branchWaterFactor * internode.m_data.m_strength * m_branchStrength;
			}
			return FLT_MAX;
		};

	shootGrowthController.m_baseNodeApicalAngle = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return glm::gaussRand(m_baseNodeApicalAngleMeanVariance.x, m_baseNodeApicalAngleMeanVariance.y);
		};

	shootGrowthController.m_internodeGrowthRate = m_growthRate / m_internodeLength;

	shootGrowthController.m_branchingAngle = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			float value = glm::gaussRand(m_branchingAngleMeanVariance.x, m_branchingAngleMeanVariance.y);
		/*
			if(const auto noise = m_branchingAngle.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}*/
			return value;
		};
	shootGrowthController.m_rollAngle = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			float value = glm::gaussRand(m_rollAngleMeanVariance.x, m_rollAngleMeanVariance.y);
		/*
			if (const auto noise = m_rollAngle.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}*/
			value += m_rollAngleNoise2D.GetValue(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance));
			return value;
		};
	shootGrowthController.m_apicalAngle = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			if (m_straightTrunk != 0.f && internode.m_data.m_order == 0 && internode.m_info.m_rootDistance < m_straightTrunk) return 0.f;
			float value = glm::gaussRand(m_apicalAngleMeanVariance.x, m_apicalAngleMeanVariance.y);
		/*
			if (const auto noise = m_apicalAngle.Get<ProceduralNoise2D>())
			{
				noise->Process(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance), value);
			}*/
			value += m_apicalAngleNoise2D.GetValue(glm::vec2(internode.GetHandle(), internode.m_info.m_rootDistance));
			return value;
		};
	shootGrowthController.m_gravitropism = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_gravitropism;
		};
	shootGrowthController.m_phototropism = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_phototropism;
		};
	shootGrowthController.m_horizontalTropism = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_horizontalTropism;
		};
	shootGrowthController.m_sagging = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			const auto newSagging = glm::min(
				m_saggingFactorThicknessReductionMax.z,
				m_saggingFactorThicknessReductionMax.x *
				internode.m_data.m_saggingForce /
				glm::pow(
					internode.m_info.m_thickness /
					m_endNodeThickness,
					m_saggingFactorThicknessReductionMax.y));
			return glm::max(internode.m_data.m_sagging, newSagging);
		};

	shootGrowthController.m_internodeStrength = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return 1.f;
		};

	shootGrowthController.m_internodeLength = m_internodeLength;
	shootGrowthController.m_internodeLengthThicknessFactor = m_internodeLengthThicknessFactor;
	shootGrowthController.m_endNodeThickness = m_endNodeThickness;
	shootGrowthController.m_thicknessAccumulationFactor = m_thicknessAccumulationFactor;
	shootGrowthController.m_thicknessAgeFactor = m_thicknessAgeFactor;
	shootGrowthController.m_internodeShadowFactor = m_internodeShadowFactor;

	shootGrowthController.m_lateralBudCount = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			if(m_maxOrder == -1 || internode.m_data.m_order < m_maxOrder)
			{
				return m_lateralBudCount;
			}
			return 0;
		};
	shootGrowthController.m_apicalBudExtinctionRate = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			if (internode.m_info.m_rootDistance < 0.5f) return 0.f;
			return m_apicalBudExtinctionRate;
		};
	shootGrowthController.m_lateralBudFlushingRate = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			float flushingRate = m_lateralBudFlushingRate;
			if (internode.m_data.m_inhibitorSink > 0.0f) flushingRate *= glm::exp(-internode.m_data.m_inhibitorSink);
			return flushingRate;
		};
	shootGrowthController.m_apicalControl = m_apicalControl;
	shootGrowthController.m_rootDistanceControl = m_rootDistanceControl;
	shootGrowthController.m_heightControl = m_heightControl;

	shootGrowthController.m_apicalDominance = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_apicalDominance * internode.m_data.m_lightIntensity;
		};
	shootGrowthController.m_apicalDominanceLoss = m_apicalDominanceLoss;


	shootGrowthController.m_leafGrowthRate = m_leafGrowthRate;
	shootGrowthController.m_fruitGrowthRate = m_fruitGrowthRate;

	shootGrowthController.m_fruitBudCount = m_fruitBudCount;
	shootGrowthController.m_leafBudCount = m_leafBudCount;

	shootGrowthController.m_leafBudFlushingProbability = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			const auto& internodeData = internode.m_data;
			const auto& probabilityRange = m_leafBudFlushingProbabilityTemperatureRange;
			float flushProbability = glm::mix(probabilityRange.x, probabilityRange.y,
				glm::clamp((internodeData.m_temperature - probabilityRange.z) / (probabilityRange.w - probabilityRange.z), 0.0f, 1.0f));
			flushProbability *= internodeData.m_lightIntensity;
			return flushProbability;
		};
	shootGrowthController.m_fruitBudFlushingProbability = [&](const SkeletonNode<InternodeGrowthData>& internode)
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
	
	shootGrowthController.m_leafFallProbability = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_leafFallProbability;
		};
	shootGrowthController.m_maxFruitSize = m_maxFruitSize;
	shootGrowthController.m_fruitPositionVariance = m_fruitPositionVariance;
	shootGrowthController.m_fruitRotationVariance = m_fruitRotationVariance;
	shootGrowthController.m_fruitDamage = [=](const SkeletonNode<InternodeGrowthData>& internode)
		{
			float fruitDamage = 0.0f;
			return fruitDamage;
		};
	shootGrowthController.m_fruitFallProbability = [&](const SkeletonNode<InternodeGrowthData>& internode)
		{
			return m_fruitFallProbability;
		};


	
}

void ShootDescriptor::Serialize(YAML::Emitter& out)
{
	out << YAML::Key << "m_baseInternodeCount" << YAML::Value << m_baseInternodeCount;
	out << YAML::Key << "m_straightTrunk" << YAML::Value << m_straightTrunk;
	out << YAML::Key << "m_baseNodeApicalAngleMeanVariance" << YAML::Value << m_baseNodeApicalAngleMeanVariance;

	out << YAML::Key << "m_growthRate" << YAML::Value << m_growthRate;
	m_rollAngle.Save("m_rollAngle", out);
	m_apicalAngle.Save("m_apicalAngle", out);

	m_rollAngleNoise2D.Save("m_rollAngleNoise2D", out);
	m_apicalAngleNoise2D.Save("m_apicalAngleNoise2D", out);

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
	out << YAML::Key << "m_maxOrder" << YAML::Value << m_maxOrder;
	out << YAML::Key << "m_apicalBudExtinctionRate" << YAML::Value << m_apicalBudExtinctionRate;
	out << YAML::Key << "m_lateralBudFlushingRate" << YAML::Value << m_lateralBudFlushingRate;
	out << YAML::Key << "m_apicalControl" << YAML::Value << m_apicalControl;
	out << YAML::Key << "m_heightControl" << YAML::Value << m_heightControl;
	out << YAML::Key << "m_rootDistanceControl" << YAML::Value << m_rootDistanceControl;

	out << YAML::Key << "m_apicalDominance" << YAML::Value << m_apicalDominance;
	out << YAML::Key << "m_apicalDominanceLoss" << YAML::Value << m_apicalDominanceLoss;

	out << YAML::Key << "m_trunkProtection" << YAML::Value << m_trunkProtection;
	out << YAML::Key << "m_maxFlowLength" << YAML::Value << m_maxFlowLength;
	out << YAML::Key << "m_lightPruningFactor" << YAML::Value << m_lightPruningFactor;
	out << YAML::Key << "m_branchStrength" << YAML::Value << m_branchStrength;
	out << YAML::Key << "m_branchStrengthThicknessFactor" << YAML::Value << m_branchStrengthThicknessFactor;
	out << YAML::Key << "m_branchStrengthLightingThreshold" << YAML::Value << m_branchStrengthLightingThreshold;
	out << YAML::Key << "m_branchStrengthLightingLoss" << YAML::Value << m_branchStrengthLightingLoss;
	out << YAML::Key << "m_branchBreakingFactor" << YAML::Value << m_branchBreakingFactor;
	out << YAML::Key << "m_branchBreakingMultiplier" << YAML::Value << m_branchBreakingMultiplier;
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
	if (in["m_baseInternodeCount"]) m_baseInternodeCount = in["m_baseInternodeCount"].as<int>();
	if (in["m_straightTrunk"]) m_straightTrunk = in["m_straightTrunk"].as<float>();
	if (in["m_baseNodeApicalAngleMeanVariance"]) m_baseNodeApicalAngleMeanVariance = in["m_baseNodeApicalAngleMeanVariance"].as<glm::vec2>();

	if (in["m_growthRate"]) m_growthRate = in["m_growthRate"].as<float>();
	m_rollAngle.Load("m_rollAngle", in);
	m_apicalAngle.Load("m_apicalAngle", in);

	m_rollAngleNoise2D.Load("m_rollAngleNoise2D", in);
	m_apicalAngleNoise2D.Load("m_apicalAngleNoise2D", in);

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
	if (in["m_maxOrder"]) m_maxOrder = in["m_maxOrder"].as<int>();
	if (in["m_apicalBudExtinctionRate"]) m_apicalBudExtinctionRate = in["m_apicalBudExtinctionRate"].as<float>();
	if (in["m_lateralBudFlushingRate"]) m_lateralBudFlushingRate = in["m_lateralBudFlushingRate"].as<float>();
	if (in["m_apicalControl"]) m_apicalControl = in["m_apicalControl"].as<float>();
	if (in["m_rootDistanceControl"]) m_rootDistanceControl = in["m_rootDistanceControl"].as<float>();
	if (in["m_heightControl"]) m_heightControl = in["m_heightControl"].as<float>();

	if (in["m_apicalDominance"]) m_apicalDominance = in["m_apicalDominance"].as<float>();
	if (in["m_apicalDominanceLoss"]) m_apicalDominanceLoss = in["m_apicalDominanceLoss"].as<float>();

	if (in["m_trunkProtection"]) m_trunkProtection = in["m_trunkProtection"].as<bool>();
	if (in["m_maxFlowLength"]) m_maxFlowLength = in["m_maxFlowLength"].as<int>();
	if (in["m_lightPruningFactor"]) m_lightPruningFactor = in["m_lightPruningFactor"].as<float>();
	if (in["m_branchStrength"]) m_branchStrength = in["m_branchStrength"].as<float>();
	if (in["m_branchStrengthThicknessFactor"]) m_branchStrengthThicknessFactor = in["m_branchStrengthThicknessFactor"].as<float>();
	if (in["m_branchStrengthLightingThreshold"]) m_branchStrengthLightingThreshold = in["m_branchStrengthLightingThreshold"].as<float>();
	if (in["m_branchStrengthLightingLoss"]) m_branchStrengthLightingLoss = in["m_branchStrengthLightingLoss"].as<float>();
	if (in["m_branchBreakingFactor"]) m_branchBreakingFactor = in["m_branchBreakingFactor"].as<float>();
	if (in["m_branchBreakingMultiplier"]) m_branchBreakingMultiplier = in["m_branchBreakingMultiplier"].as<float>();

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
	changed = ImGui::DragFloat("Straight Trunk", &m_straightTrunk, 0.1f, 0.0f, 100.f) || changed;
	if (ImGui::TreeNodeEx("Internode", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed = ImGui::DragInt("Base node count", &m_baseInternodeCount, 1, 0, 3) || changed;
		changed = ImGui::DragFloat2("Base apical angle mean/var", &m_baseNodeApicalAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;

		changed = ImGui::DragInt("Lateral bud count", &m_lateralBudCount, 1, 0, 3) || changed;
		changed = ImGui::DragInt("Max Order", &m_maxOrder, 1, -1, 100) || changed;
		if (ImGui::TreeNodeEx("Angles"))
		{
			changed = ImGui::DragFloat2("Branching angle base/var", &m_branchingAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
			//editorLayer->DragAndDropButton<ProceduralNoise2D>(m_branchingAngle, "Branching Angle Noise");
			changed = ImGui::DragFloat2("Roll angle base/var", &m_rollAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
			//editorLayer->DragAndDropButton<ProceduralNoise2D>(m_rollAngle, "Roll Angle Noise");
			changed = ImGui::DragFloat2("Apical angle base/var", &m_apicalAngleMeanVariance.x, 0.1f, 0.0f, 100.0f) || changed;
			//editorLayer->DragAndDropButton<ProceduralNoise2D>(m_apicalAngle, "Apical Angle Noise");
			if (ImGui::TreeNodeEx("Roll Angle Noise2D"))
			{
				changed = m_rollAngleNoise2D.OnInspect() | changed;
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Apical Angle Noise2D"))
			{
				changed = m_apicalAngleNoise2D.OnInspect() | changed;
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
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

		changed = ImGui::DragFloat("Apical bud extinction rate", &m_apicalBudExtinctionRate, 0.01f, 0.0f, 1.0f, "%.5f") || changed;
		changed = ImGui::DragFloat("Lateral bud flushing rate", &m_lateralBudFlushingRate, 0.01f, 0.0f, 1.0f, "%.5f") || changed;
		
		changed = ImGui::DragFloat2("Inhibitor val/loss", &m_apicalDominance, 0.01f) || changed;
		ImGui::TreePop();
	}
	if(ImGui::TreeNodeEx("Tree Shape Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed = ImGui::DragFloat("Apical control", &m_apicalControl, 0.01f) || changed;
		changed = ImGui::DragFloat("Root distance control", &m_rootDistanceControl, 0.01f) || changed;
		changed = ImGui::DragFloat("Height control", &m_heightControl, 0.01f) || changed;

		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Pruning", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed = ImGui::Checkbox("Trunk Protection", &m_trunkProtection) || changed;
		changed = ImGui::DragInt("Max chain length", &m_maxFlowLength, 1) || changed;
		changed = ImGui::DragFloat("Light pruning threshold", &m_lightPruningFactor, 0.01f) || changed;
		
		changed = ImGui::DragFloat("Branch strength", &m_branchStrength, 0.01f, 0.0f) || changed;
		changed = ImGui::DragFloat("Branch strength thickness factor", &m_branchStrengthThicknessFactor, 0.01f, 0.0f) || changed;
		changed = ImGui::DragFloat("Branch strength lighting threshold", &m_branchStrengthLightingThreshold, 0.01f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat("Branch strength lighting loss", &m_branchStrengthLightingLoss, 0.01f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat("Branch breaking multiplier", &m_branchBreakingMultiplier, 0.01f, 0.01f, 10.0f) || changed;

		changed = ImGui::DragFloat("Branch breaking factor", &m_branchBreakingFactor, 0.01f, 0.01f, 10.0f) || changed;


		ImGui::TreePop();
	}
	changed = ImGui::DragInt("Leaf bud count", &m_leafBudCount, 1, 0, 3) || changed;
	if (m_leafBudCount > 0 && ImGui::TreeNodeEx("Leaf"))
	{
		changed = ImGui::DragFloat("Leaf growth rate", &m_leafGrowthRate, 0.01f, 0.0f, 1.0f) || changed;
		changed = ImGui::DragFloat4("Leaf flushing prob/temp range", &m_leafBudFlushingProbabilityTemperatureRange.x, 0.01f, 0.0f, 1.0f, "%.5f") || changed;
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
		changed = ImGui::DragFloat4("Fruit flushing prob/temp range", &m_fruitBudFlushingProbabilityTemperatureRange.x, 0.01f, 0.0f, 1.0f, "%.5f") || changed;
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
	if (m_rollAngle.Get<ProceduralNoise2D>()) list.push_back(m_rollAngle);
	if (m_apicalAngle.Get<ProceduralNoise2D>()) list.push_back(m_apicalAngle);

	if (m_barkMaterial.Get<Material>()) list.push_back(m_barkMaterial);
}
