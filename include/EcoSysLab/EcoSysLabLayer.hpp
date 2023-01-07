#pragma once

#include "ecosyslab_export.h"
#include "TreeModel.hpp"
#include "TreeVisualizer.hpp"
#include "TreeMeshGenerator.hpp"
using namespace UniEngine;
namespace EcoSysLab {
	class EcoSysLabLayer : public ILayer {
		bool m_displayBranches = true;
		bool m_displayRootFlows = true;
		bool m_displayBoundingBox = false;
		bool m_displaySoil = true;

		bool m_debugVisualization = true;
		bool m_rendering = false;
		std::vector<int> m_versions;
		std::vector<glm::vec3> m_randomColors;
		
		std::vector<glm::uint> m_branchSegments;
		std::vector<StrandPoint> m_branchPoints;
		std::vector<glm::uint> m_rootSegments;
		std::vector<StrandPoint> m_rootPoints;

		AssetRef m_branchStrands;
		AssetRef m_rootStrands;

		std::vector<glm::mat4> m_boundingBoxMatrices;
		std::vector<glm::vec4> m_boundingBoxColors;

		float m_lastUsedTime = 0.0f;
		float m_totalTime = 0.0f;
		int m_internodeSize = 0;
		int m_branchSize = 0;
		int m_rootNodeSize = 0;
		int m_rootFlowSize = 0;
		bool m_needFlowUpdate = false;
		bool m_lockTreeSelection = false;
		bool m_autoGrow = false;


		int m_soilVersion = -1;
		bool m_vectorEnable = true;
		bool m_scalarEnable = true;
		bool m_updateVectorMatrices = false;
		bool m_updateVectorColors = false;
		bool m_updateScalarMatrices = false;
		bool m_updateScalarColors = false;
		float m_vectorMultiplier = 50.0f;
		glm::vec4 m_vectorBaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
		unsigned m_vectorSoilProperty = 4;
		float m_vectorLineWidthFactor = 0.1f;
		float m_vectorLineMaxWidth = 0.1f;
		std::vector<glm::mat4> m_vectorMatrices;
		std::vector<glm::vec4> m_vectorColors;

		float m_scalarMultiplier = 1.0f;
		float m_scalarBoxSize = 0.5f;
		float m_scalarMinAlpha = 0.00f;
		glm::vec3 m_scalarBaseColor = glm::vec3(0.0f, 0.0f, 1.0f);
		unsigned m_scalarSoilProperty = 0;
		std::vector<glm::mat4> m_scalarMatrices;
		std::vector<glm::vec4> m_scalarColors;

		void FixedUpdate() override;

		void OnCreate() override;

		void OnDestroy() override;

		void LateUpdate() override;

		void OnInspect() override;

		void OnSoilVisualizationMenu();

		std::shared_ptr<OpenGLUtils::GLShader> m_treeBranchComp;
		std::shared_ptr<OpenGLUtils::GLProgram> m_treeBranchComputeProgram;
		std::unique_ptr<OpenGLUtils::GLBuffer> m_treeBranchBuffer;
		std::shared_ptr<Mesh> m_treeMesh;
	public:
		TreeMeshGeneratorSettings m_meshGeneratorSettings;
		Entity m_selectedTree = {};
		
		EntityRef m_branchStrandsHolder;
		EntityRef m_rootStrandsHolder;
		TreeVisualizer m_treeVisualizer;

		PrivateComponentRef m_soilHolder;
		PrivateComponentRef m_climateHolder;
		void GrowAllTrees();
		void GenerateMeshes(const TreeMeshGeneratorSettings& meshGeneratorSettings);

	};
}