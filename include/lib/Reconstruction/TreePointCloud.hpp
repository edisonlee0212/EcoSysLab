#pragma once

#include "VoxelGrid.hpp"
#include "Skeleton.hpp"
#include "TreeMeshGenerator.hpp"
#include "Curve.hpp"

using namespace EvoEngine;
namespace EcoSysLab {
	typedef int PointHandle;
	typedef int BranchHandle;
	typedef int TreePartHandle;
	struct ScatteredPoint {
		PointHandle m_handle = -1;
		std::vector<PointHandle> m_neighborScatterPoints;

		std::vector<std::pair<float, BranchHandle>> m_neighborBranchP3s;
		glm::vec3 m_position = glm::vec3(0.0f);
	};
	struct AllocatedPoint {
		glm::vec3 m_color;
		glm::vec3 m_position;
		PointHandle m_handle = -1;
		TreePartHandle m_treePartHandle = -1;
		BranchHandle m_branchHandle = -1;
		NodeHandle m_nodeHandle = -1;
		int m_skeletonIndex = -1;
	};
	struct ScannedBranch {
		glm::vec3 m_color;

		TreePartHandle m_treePartHandle = -1;
		BranchHandle m_handle = -1;
		BezierCurve m_bezierCurve;
		float m_startThickness = 0.0f;
		float m_endThickness = 0.0f;
		std::vector<std::pair<float, PointHandle>> m_neighborScatterPointP0;

		std::unordered_map<BranchHandle, float> m_neighborBranchP3s;

	};

	struct OperatingBranch {
		glm::vec3 m_color;

		TreePartHandle m_treePartHandle = -1;
		BranchHandle m_handle = -1;
		BezierCurve m_bezierCurve;
		float m_startThickness = 0.0f;
		float m_endThickness = 0.0f;

		BranchHandle m_parentHandle = -1;
		std::vector<BranchHandle> m_childHandles;

		int m_skeletonIndex = -1;
		std::vector<NodeHandle> m_chainNodeHandles;

		std::vector<BranchHandle> m_parentCandidates;
		bool m_used = false;

		void Apply(const ScannedBranch& target);
	};

	struct TreePart {
		glm::vec3 m_color;
		TreePartHandle m_handle = -1;
		std::vector<PointHandle> m_allocatedPoints;
		std::vector<BranchHandle> m_branchHandles;
	};

	struct ConnectivityGraphSettings {
		float m_pointPointConnectionDetectionRadius = 0.05f;
		float m_pointBranchConnectionDetectionRange = 0.5f;
		float m_branchBranchConnectionMaxLengthRange = 1.0f;
		float m_directionConnectionAngleLimit = 30.0f;
		float m_indirectConnectionAngleLimit = 90.0f;
		bool m_checkReverse = true;
		float m_branchShortening = 0.0f;
		float m_voxelSize = 0.05f;

		void OnInspect();
	};

	struct ScatterPointData {
		glm::vec3 m_position = glm::vec3(0.0f);
		int m_pointHandle = -1;
	};
	struct BranchEndData {
		bool m_isP0 = true;
		glm::vec3 m_position = glm::vec3(0.0f);
		int m_branchHandle = -1;
	};

	struct ReconstructionSettings {
		float m_internodeLength = 0.03f;
		float m_minHeight = 0.3f;
		float m_minimumTreeDistance = 0.1f;
		float m_branchShortening = 0.3f;

		float m_endNodeThickness = 0.002f;
		float m_thicknessSumFactor = 0.4f;
		float m_thicknessAccumulationFactor = 0.00005f;
		bool m_overrideThickness = true;
		bool m_limitParentThickness = true;
		int m_minimumNodeCount = 1;

		float m_minimumRootThickness = 0.02f;

		
		//float m_angleLimit = 90.0f;
		void OnInspect();
	};

	struct ReconstructionSkeletonData {
		glm::vec3 m_rootPosition = glm::vec3(0.0f);
	};
	struct ReconstructionFlowData {

	};
	struct ReconstructionNodeData {
		glm::vec3 m_globalEndPosition = glm::vec3(0.0f);


		std::vector<PointHandle> m_allocatedPoints;
		std::vector<PointHandle> m_filteredPoints;
		BranchHandle m_branchHandle;
	};
	typedef Skeleton<ReconstructionSkeletonData, ReconstructionFlowData, ReconstructionNodeData> ReconstructionSkeleton;

	class TreePointCloud : public IPrivateComponent {
		void FindPoints(const glm::vec3& position, VoxelGrid<std::vector<ScatterPointData>>& pointVoxelGrid, float radius,
			const std::function<void(const ScatterPointData& voxel)>& func) const;
		void ForEachBranchEnd(const glm::vec3& position, VoxelGrid<std::vector<BranchEndData>>& branchEndsVoxelGrid, float radius,
			const std::function<void(const BranchEndData& voxel)>& func) const;

		void CalculateNodeTransforms(ReconstructionSkeleton& skeleton);

		void BuildConnectionBranch(
			ReconstructionSkeleton& skeleton, const BranchHandle processingBranchHandle, NodeHandle& prevNodeHandle);
		void AttachBranch();
	public:
		TreeMeshGeneratorSettings m_treeMeshGeneratorSettings {};
		ReconstructionSettings m_reconstructionSettings{};
		ConnectivityGraphSettings m_connectivityGraphSettings{};
		void ImportGraph(const std::filesystem::path& path, float scaleFactor = 0.1f);
		void ExportForestOBJ(const std::filesystem::path& path);
		glm::vec3 m_min;
		glm::vec3 m_max;
		std::vector<ScatteredPoint> m_scatteredPoints;
		std::vector<AllocatedPoint> m_allocatedPoints;
		std::vector<ScannedBranch> m_scannedBranches;

		std::vector<OperatingBranch> m_operatingBranches;
		std::vector<TreePart> m_treeParts;

		void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;

		std::vector<ReconstructionSkeleton> m_skeletons;

		std::vector<std::pair<glm::vec3, glm::vec3>> m_scatterPointToBranchEndConnections;
		std::vector<std::pair<glm::vec3, glm::vec3>> m_scatterPointToBranchStartConnections;
		std::vector<std::pair<glm::vec3, glm::vec3>> m_scatterPointsConnections;
		std::vector<std::pair<glm::vec3, glm::vec3>> m_branchConnections;
		std::vector<std::pair<glm::vec3, glm::vec3>> m_filteredBranchConnections;

		void EstablishConnectivityGraph();

		void BuildSkeletons();

		void ClearMeshes() const;

		void OnCreate() override;

		void FormGeometryEntity() const;

		std::vector<std::shared_ptr<Mesh>> GenerateForestBranchMeshes() const;
		std::vector<std::shared_ptr<Mesh>> GenerateFoliageMeshes() const;
	};
}