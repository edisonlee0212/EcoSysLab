#pragma once

#include "TreeVisualizer.hpp"
#include "TreeMeshGenerator.hpp"
#include "LSystemString.hpp"
#include "ParticlePhysics2D.hpp"
#include "PipeModelData.hpp"
#include "TreeGraph.hpp"
#include "TreeGrowthParameters.hpp"
using namespace EvoEngine;
namespace EcoSysLab
{
	struct TreePipeProfile
	{
		ParticlePhysics2D<CellParticlePhysicsData> m_particlePhysics2D;
		std::unordered_map<PipeHandle, ParticleHandle> m_particleMap{};
		float m_a = 0.0f;
	};
	class TreePipeNode : public IPrivateComponent
	{
	public:
		std::vector<std::shared_ptr<TreePipeProfile>> m_profiles {};
		PipeHandle m_pipeHandle = -1;

		float m_frontControlPointDistance = 0.0f;
		float m_backControlPointDistance = 0.0f;

		float m_centerDirectionRadius = 0.0f;

		bool m_apical = false;

		void InsertInterpolation(float a);
		void OnDestroy() override;
		void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;
	};
}