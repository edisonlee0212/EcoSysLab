#pragma once
#include "StrandModelProfile.hpp"
#include "ProfileConstraints.hpp"
using namespace EvoEngine;
namespace EcoSysLab
{
	struct ParticlePhysicsDemoData
	{
		glm::vec4 m_color = glm::vec4(1.0f);
	};
	class ParticlePhysics2DDemo : public IPrivateComponent
	{
		StrandModelProfile<ParticlePhysicsDemoData> m_particlePhysics2D;
		ProfileConstraints m_profileBoundaries;
		bool m_boundariesUpdated = false;
	public:
		glm::vec2 m_worldCenter = glm::vec2(0.0f);
		float m_worldRadius = 100.0f;
		float m_gravityStrength = 10.0f;
		int m_particleAddCount = 10;
		void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;
		void FixedUpdate() override;
	};
}