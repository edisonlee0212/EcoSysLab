#pragma once


#include "Mesh.hpp"
#include "Material.hpp"

namespace EvoEngine {
	class BillboardCloud {
	public:
		struct ClusterTriangle
		{
			int m_elementIndex = -1;
			int m_triangleIndex = -1;

			int m_index;
		};
		struct ProjectedTriangle
		{
			Vertex m_projectedVertices[3];
			Handle m_materialHandle;
		};

		struct Element {
			/**
			 * Vertices must be in model space.
			 */
			std::vector<Vertex> m_vertices;
			std::shared_ptr<Material> m_material;
			std::vector<glm::uvec3> m_triangles;

			[[nodiscard]] glm::vec3 CalculateCentroid(int triangleIndex) const;
			[[nodiscard]] float CalculateArea(int triangleIndex) const;
			[[nodiscard]] float CalculateNormalDistance(int triangleIndex) const;
			[[nodiscard]] glm::vec3 CalculateNormal(int triangleIndex) const;

			[[nodiscard]] glm::vec3 CalculateCentroid(const glm::uvec3& triangle) const;
			[[nodiscard]] float CalculateArea(const glm::uvec3& triangle) const;
			[[nodiscard]] glm::vec3 CalculateNormal(const glm::uvec3& triangle) const;
			[[nodiscard]] float CalculateNormalDistance(const glm::uvec3& triangle) const;
		};

		struct Rectangle
		{
			glm::vec2 m_points[4];

			glm::vec2 m_texCoords[4];

			void Update();

			glm::vec2 m_center;
			glm::vec2 m_xAxis;
			glm::vec2 m_yAxis;

			float m_width;
			float m_height;

			[[nodiscard]] glm::vec2 Transform(const glm::vec2& target) const;
			[[nodiscard]] glm::vec3 Transform(const glm::vec3& target) const;
		};
		struct ProjectSettings
		{
			
		};
		
		struct JoinSettings
		{
			glm::uvec2 m_resolution = glm::uvec2(2048);
		};

		struct RasterizeSettings
		{
			bool m_transferAlbedoMap = true;
			bool m_transferNormalMap = true;
			bool m_transferRoughnessMap = false;
			bool m_transferMetallicMap = false;
			bool m_transferAoMap = false;

			glm::uvec2 m_resolution = glm::uvec2(2048);
		};
		struct PBRMaterial
		{
			glm::vec4 m_baseAlbedo = glm::vec4(1.f);
			float m_baseRoughness = 0.3f;
			float m_baseMetallic = 0.3f;
			float m_baseAo = 1.f;
			glm::ivec2 m_albedoTextureResolution = glm::ivec2(-1);
			std::vector<glm::vec4> m_albedoTextureData;

			glm::ivec2 m_normalTextureResolution = glm::ivec2(-1);
			std::vector<glm::vec3> m_normalTextureData;

			glm::ivec2 m_roughnessTextureResolution = glm::ivec2(-1);
			std::vector<float> m_roughnessTextureData;

			glm::ivec2 m_metallicTextureResolution = glm::ivec2(-1);
			std::vector<float> m_metallicTextureData;

			glm::ivec2 m_aoTextureResolution = glm::ivec2(-1);
			std::vector<float> m_aoTextureData;
			void Clear();
			void ApplyMaterial(const std::shared_ptr<Material>& material, const RasterizeSettings& rasterizeSettings);
		};
		struct Cluster
		{
			Plane m_clusterPlane = Plane(glm::vec3(0, 0, 1), 0.f);

			std::vector<ClusterTriangle> m_triangles;

			std::vector<ProjectedTriangle> m_projectedTriangles;
			/**
			 * Billboard's bounding rectangle.
			 */
			Rectangle m_rectangle;
			/**
			 * Billboard's corresponding mesh.
			 */
			std::vector<Vertex> m_billboardVertices;
			std::vector<glm::uvec3> m_billboardTriangles;
			
		};

		enum class ClusterizationMode
		{
			PassThrough,
			Default,
			Stochastic
		};

		struct StochasticClusterizationSettings
		{
			float m_epsilon = 0.3f;
			int m_iteration = 200;
			int m_timeout = 300;
			float m_maxPlaneSize = 0.1f;
		};
		struct DefaultClusterizationSettings
		{
			float m_epsilonPercentage = 0.01f;
			int m_thetaNum = 10;
			int m_phiNum = 10;
			int m_timeout = 300;
		};

		struct ClusterizationSettings
		{
			bool m_append = true;
			StochasticClusterizationSettings m_stochasticClusterizationSettings{};
			DefaultClusterizationSettings m_defaultClusterizationSettings{};
			ClusterizationMode m_clusterizeMode = ClusterizationMode::PassThrough;
		};

		class ElementCollection {
			
			void Project(Cluster& cluster, const ProjectSettings& projectSettings) const;
			std::vector<Cluster> StochasticClusterize(std::vector<ClusterTriangle> operatingTriangles, const ClusterizationSettings& clusterizeSettings);
			std::vector<Cluster> DefaultClusterize(std::vector<ClusterTriangle> operatingTriangles, const ClusterizationSettings& clusterizeSettings);
			
		public:
			std::vector<ClusterTriangle> m_skippedTriangles;

			std::vector<Element> m_elements;
			std::vector<Cluster> m_clusters;

			[[nodiscard]] glm::vec3 CalculateCentroid(const ClusterTriangle& triangle) const;
			[[nodiscard]] float CalculateArea(const ClusterTriangle& triangle) const;
			[[nodiscard]] float CalculateNormalDistance(const ClusterTriangle& triangle) const;
			[[nodiscard]] glm::vec3 CalculateNormal(const ClusterTriangle& triangle) const;


			std::shared_ptr<Mesh> m_billboardCloudMesh;
			std::shared_ptr<Material> m_billboardCloudMaterial;

			void Clusterize(const ClusterizationSettings& clusterizeSettings);
			void Project(const ProjectSettings& projectSettings);
			void Join(const JoinSettings& joinSettings);
			void Rasterize(const RasterizeSettings& rasterizeSettings);
		};

		/**
		 * Each element collection corresponding to a group of mesh material combinations.
		 * The primitives are grouped together to be clustered.
		 */
		std::vector<ElementCollection> m_elementCollections{};

		void ProcessPrefab(const std::shared_ptr<Prefab>& prefab, bool combine);

		[[nodiscard]] Entity BuildEntity(const std::shared_ptr<Scene>& scene) const;
	private:

		static void PreprocessPrefab(std::vector<ElementCollection>& elementCollections, const std::shared_ptr<Prefab>& currentPrefab, const Transform& parentModelSpaceTransform);
		static void PreprocessPrefab(ElementCollection& elementCollection, const std::shared_ptr<Prefab>& currentPrefab, const Transform& parentModelSpaceTransform);

		//Adopted from https://github.com/DreamVersion/RotatingCalipers
		class RotatingCalipers
		{
		public:
			static std::vector<glm::vec2> ConvexHull(std::vector<glm::vec2> points);
			static Rectangle GetMinAreaRectangle(std::vector<glm::vec2> points);
		};
	};
}