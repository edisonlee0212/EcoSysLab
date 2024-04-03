#ifdef BUILD_WITH_RAYTRACER
#include "BTFMeshRenderer.hpp"
#include "RayTracerLayer.hpp"
#include <TriangleIlluminationEstimator.hpp>
#endif
#include "ClassRegistry.hpp"
#include "Graphics.hpp"
#include "SkyIlluminance.hpp"
#include "SorghumDescriptor.hpp"
#include <SorghumLayer.hpp>
#include "Times.hpp"

#include "Material.hpp"
#include "Sorghum.hpp"
#include "SorghumCoordinates.hpp"
#include "SorghumPointCloudScanner.hpp"
#ifdef BUILD_WITH_RAYTRACER
#include "CBTFGroup.hpp"
#include "DoubleCBTF.hpp"
#include "PARSensorGroup.hpp"
#endif
using namespace EcoSysLab;
using namespace EvoEngine;

void SorghumLayer::OnCreate() {
	ClassRegistry::RegisterAsset<SorghumState>("SorghumState", { ".sorghumstate" });
	ClassRegistry::RegisterPrivateComponent<Sorghum>("Sorghum");
	ClassRegistry::RegisterAsset<SorghumGrowthDescriptor>("SorghumGrowthDescriptor",
		{ ".sorghumgrowth" });
	ClassRegistry::RegisterAsset<SorghumDescriptor>(
		"SorghumDescriptor", { ".sorghum" });
	ClassRegistry::RegisterAsset<SorghumField>("SorghumField", { ".sorghumfield" });
#ifdef BUILD_WITH_RAYTRACER
	ClassRegistry::RegisterAsset<PARSensorGroup>("PARSensorGroup",
		{ ".parsensorgroup" });
	ClassRegistry::RegisterAsset<CBTFGroup>("CBTFGroup", { ".cbtfg" });
	ClassRegistry::RegisterAsset<DoubleCBTF>("DoubleCBTF", { ".dcbtf" });
#endif
	ClassRegistry::RegisterAsset<SkyIlluminance>("SkyIlluminance",
		{ ".skyilluminance" });
	ClassRegistry::RegisterAsset<SorghumCoordinates>("SorghumCoordinates",
		{ ".sorghumcoords" });
	ClassRegistry::RegisterPrivateComponent<SorghumPointCloudScanner>("SorghumPointCloudScanner");
	if (const auto editorLayer = Application::GetLayer<EditorLayer>()) {
		auto texture2D = ProjectManager::CreateTemporaryAsset<Texture2D>();
		texture2D->Import(std::filesystem::absolute(
			std::filesystem::path("./EcoSysLabResources/Textures") /
			"SorghumGrowthDescriptor.png"));
		editorLayer->AssetIcons()["SorghumGrowthDescriptor"] = texture2D;
		texture2D = ProjectManager::CreateTemporaryAsset<Texture2D>();
		texture2D->Import(std::filesystem::absolute(
			std::filesystem::path("./EcoSysLabResources/Textures") /
			"SorghumDescriptor.png"));
		editorLayer->AssetIcons()["SorghumDescriptor"] = texture2D;
		texture2D = ProjectManager::CreateTemporaryAsset<Texture2D>();
		texture2D->Import(std::filesystem::absolute(
			std::filesystem::path("./EcoSysLabResources/Textures") /
			"PositionsField.png"));
		editorLayer->AssetIcons()["PositionsField"] = texture2D;

		texture2D->Import(std::filesystem::absolute(
			std::filesystem::path("./EcoSysLabResources/Textures") /
			"GeneralDataPipeline.png"));
		editorLayer->AssetIcons()["GeneralDataCapture"] = texture2D;
	}
	
	if (!m_leafAlbedoTexture.Get<Texture2D>()) {
		const auto albedo = ProjectManager::CreateTemporaryAsset<Texture2D>();
		albedo->Import(std::filesystem::absolute(
			std::filesystem::path("./EcoSysLabResources/Textures") /
			"leafSurface.jpg"));
		m_leafAlbedoTexture.Set(albedo);
	}

	if (!m_leafMaterial.Get<Material>()) {
		const auto material = ProjectManager::CreateTemporaryAsset<Material>();
		m_leafMaterial = material;
		material->SetAlbedoTexture(m_leafAlbedoTexture.Get<Texture2D>());
		material->m_materialProperties.m_albedoColor =
			glm::vec3(113.0f / 255, 169.0f / 255, 44.0f / 255);
		material->m_materialProperties.m_roughness = 0.8f;
		material->m_materialProperties.m_metallic = 0.1f;
	}

	if (!m_leafBottomFaceMaterial.Get<Material>()) {
		const auto material = ProjectManager::CreateTemporaryAsset<Material>();
		m_leafBottomFaceMaterial = material;
		material->SetAlbedoTexture(m_leafAlbedoTexture.Get<Texture2D>());
		material->m_materialProperties.m_albedoColor =
			glm::vec3(113.0f / 255, 169.0f / 255, 44.0f / 255);
		material->m_materialProperties.m_roughness = 0.8f;
		material->m_materialProperties.m_metallic = 0.1f;
	}

	if (!m_panicleMaterial.Get<Material>()) {
		const auto material = ProjectManager::CreateTemporaryAsset<Material>();
		m_panicleMaterial = material;
		material->m_materialProperties.m_albedoColor =
			glm::vec3(255.0 / 255, 210.0 / 255, 0.0 / 255);
		material->m_materialProperties.m_roughness = 0.5f;
		material->m_materialProperties.m_metallic = 0.0f;
	}

	for (auto& i : m_segmentedLeafMaterials) {
		if (!i.Get<Material>()) {
			const auto material = ProjectManager::CreateTemporaryAsset<Material>();
			i = material;
			material->m_materialProperties.m_albedoColor =
				glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f));
			material->m_materialProperties.m_roughness = 1.0f;
			material->m_materialProperties.m_metallic = 0.0f;
		}
	}
}


void SorghumLayer::GenerateMeshForAllSorghums(const SorghumMeshGeneratorSettings& sorghumMeshGeneratorSettings) const
{
	std::vector<Entity> plants;
	const auto scene = GetScene();
	if (const std::vector<Entity>* sorghumEntities =
		scene->UnsafeGetPrivateComponentOwnersList<Sorghum>(); sorghumEntities && !sorghumEntities->empty())
	{
		for (const auto& sorghumEntity : *sorghumEntities)
		{
			const auto sorghum = scene->GetOrSetPrivateComponent<Sorghum>(sorghumEntity).lock();
			sorghum->GenerateGeometryEntities(sorghumMeshGeneratorSettings);
		}
	}
}

void SorghumLayer::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) {
	auto scene = GetScene();
	if (ImGui::Begin("Sorghum Layer")) {
#ifdef BUILD_WITH_RAYTRACER
		if (ImGui::TreeNodeEx("Illumination Estimation")) {
			ImGui::DragInt("Seed", &m_seed);
			ImGui::DragFloat("Push distance along normal", &m_pushDistance, 0.0001f,
				-1.0f, 1.0f, "%.5f");
			m_rayProperties.OnInspect();

			if (ImGui::Button("Calculate illumination")) {
				CalculateIlluminationFrameByFrame();
			}
			if (ImGui::Button("Calculate illumination instantly")) {
				CalculateIllumination();
			}
			ImGui::TreePop();
		}
		editorLayer->DragAndDropButton<CBTFGroup>(m_leafCBTFGroup,
			"Leaf CBTFGroup");

		ImGui::Checkbox("Enable BTF", &m_enableCompressedBTF);
#endif
		ImGui::Separator();
		ImGui::Checkbox("Auto regenerate sorghum", &m_autoRefreshSorghums);
		m_sorghumMeshGeneratorSettings.OnInspect(editorLayer);
		if (ImGui::Button("Generate mesh for all sorghums")) {
			GenerateMeshForAllSorghums(m_sorghumMeshGeneratorSettings);
		}
		if (ImGui::DragFloat("Vertical subdivision max unit length",
			&m_verticalSubdivisionLength, 0.001f, 0.001f,
			1.0f, "%.4f")) {
			m_verticalSubdivisionLength =
				glm::max(0.0001f, m_verticalSubdivisionLength);
		}

		if (ImGui::DragInt("Horizontal subdivision step",
			&m_horizontalSubdivisionStep)) {
			m_horizontalSubdivisionStep = glm::max(2, m_horizontalSubdivisionStep);
		}

		if (ImGui::DragFloat("Skeleton width", &m_skeletonWidth, 0.001f, 0.001f,
			1.0f, "%.4f")) {
			m_skeletonWidth = glm::max(0.0001f, m_skeletonWidth);
		}
		ImGui::ColorEdit3("Skeleton color", &m_skeletonColor.x);

		if (editorLayer->DragAndDropButton<Texture2D>(m_leafAlbedoTexture,
			"Replace Leaf Albedo Texture")) {
			auto tex = m_leafAlbedoTexture.Get<Texture2D>();
			if (tex) {
				m_leafMaterial.Get<Material>()->SetAlbedoTexture(m_leafAlbedoTexture.Get<Texture2D>());
				std::vector<Entity> sorghumEntities;

				for (const auto& i : sorghumEntities) {
					if (scene->HasPrivateComponent<MeshRenderer>(i)) {
						scene->GetOrSetPrivateComponent<MeshRenderer>(i)
							.lock()
							->m_material.Get<Material>()
							->SetAlbedoTexture(m_leafAlbedoTexture.Get<Texture2D>());
					}
				}
			}
		}

		if (editorLayer->DragAndDropButton<Texture2D>(m_leafNormalTexture,
			"Replace Leaf Normal Texture")) {
			auto tex = m_leafNormalTexture.Get<Texture2D>();
			if (tex) {
				m_leafMaterial.Get<Material>()->SetNormalTexture(m_leafNormalTexture.Get<Texture2D>());
				std::vector<Entity> sorghumEntities;

				for (const auto& i : sorghumEntities) {
					if (scene->HasPrivateComponent<MeshRenderer>(i)) {
						scene->GetOrSetPrivateComponent<MeshRenderer>(i)
							.lock()
							->m_material.Get<Material>()
							->SetNormalTexture(m_leafNormalTexture.Get<Texture2D>());
					}
				}
			}
		}

		FileUtils::SaveFile("Export OBJ for all sorghums", "3D Model", { ".obj" },
			[this](const std::filesystem::path& path) {
				ExportAllSorghumsModel(path.string());
			});

		static bool opened = false;
#ifdef BUILD_WITH_RAYTRACER
		if (m_processing && !opened) {
			ImGui::OpenPopup("Illumination Estimation");
			opened = true;
		}
		if (ImGui::BeginPopupModal("Illumination Estimation", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Progress: ");
			float fraction = 1.0f - static_cast<float>(m_processingIndex) /
				m_processingEntities.size();
			std::string text =
				std::to_string(static_cast<int>(fraction * 100.0f)) + "% - " +
				std::to_string(m_processingEntities.size() - m_processingIndex) +
				"/" + std::to_string(m_processingEntities.size());
			ImGui::ProgressBar(fraction, ImVec2(240, 0), text.c_str());
			ImGui::SetItemDefaultFocus();
			ImGui::Text(("Estimation time for 1 plant: " +
				std::to_string(m_perPlantCalculationTime) + " seconds")
				.c_str());
			if (ImGui::Button("Cancel") || m_processing == false) {
				m_processing = false;
				opened = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
}
#endif
  }
	ImGui::End();
}

void SorghumLayer::ExportSorghum(const Entity& sorghum, std::ofstream& of,
	unsigned& startIndex) {
	auto scene = Application::GetActiveScene();
	const std::string start = "#Sorghum\n";
	of.write(start.c_str(), start.size());
	of.flush();
	const auto position =
		scene->GetDataComponent<GlobalTransform>(sorghum).GetPosition();

	const auto stemMesh = scene->GetOrSetPrivateComponent<MeshRenderer>(sorghum)
		.lock()
		->m_mesh.Get<Mesh>();
	ObjExportHelper(position, stemMesh, of, startIndex);

	scene->ForEachDescendant(sorghum, [&](Entity child) {
		if (!scene->HasPrivateComponent<MeshRenderer>(child))
			return;
		const auto leafMesh = scene->GetOrSetPrivateComponent<MeshRenderer>(child)
			.lock()
			->m_mesh.Get<Mesh>();
		ObjExportHelper(position, leafMesh, of, startIndex);
		});
}

void SorghumLayer::ObjExportHelper(glm::vec3 position,
	std::shared_ptr<Mesh> mesh,
	std::ofstream& of, unsigned& startIndex) {
	if (mesh && !mesh->UnsafeGetTriangles().empty()) {
		std::string header =
			"#Vertices: " + std::to_string(mesh->GetVerticesAmount()) +
			", tris: " + std::to_string(mesh->GetTriangleAmount());
		header += "\n";
		of.write(header.c_str(), header.size());
		of.flush();
		std::string o = "o ";
		o += "[" + std::to_string(position.x) + "," + std::to_string(position.z) +
			"]" + "\n";
		of.write(o.c_str(), o.size());
		of.flush();
		std::string data;
#pragma region Data collection

		for (auto i = 0; i < mesh->UnsafeGetVertices().size(); i++) {
			auto& vertexPosition = mesh->UnsafeGetVertices().at(i).m_position;
			auto& color = mesh->UnsafeGetVertices().at(i).m_color;
			data += "v " + std::to_string(vertexPosition.x + position.x) + " " +
				std::to_string(vertexPosition.y + position.y) + " " +
				std::to_string(vertexPosition.z + position.z) + " " +
				std::to_string(color.x) + " " + std::to_string(color.y) + " " +
				std::to_string(color.z) + "\n";
		}
		for (const auto& vertex : mesh->UnsafeGetVertices()) {
			data += "vn " + std::to_string(vertex.m_normal.x) + " " +
				std::to_string(vertex.m_normal.y) + " " +
				std::to_string(vertex.m_normal.z) + "\n";
		}

		for (const auto& vertex : mesh->UnsafeGetVertices()) {
			data += "vt " + std::to_string(vertex.m_texCoord.x) + " " +
				std::to_string(vertex.m_texCoord.y) + "\n";
		}
		// data += "s off\n";
		data += "# List of indices for faces vertices, with (x, y, z).\n";
		auto& triangles = mesh->UnsafeGetTriangles();
		for (auto i = 0; i < mesh->GetTriangleAmount(); i++) {
			const auto triangle = triangles[i];
			const auto f1 = triangle.x + startIndex;
			const auto f2 = triangle.y + startIndex;
			const auto f3 = triangle.z + startIndex;
			data += "f " + std::to_string(f1) + "/" + std::to_string(f1) + "/" +
				std::to_string(f1) + " " + std::to_string(f2) + "/" +
				std::to_string(f2) + "/" + std::to_string(f2) + " " +
				std::to_string(f3) + "/" + std::to_string(f3) + "/" +
				std::to_string(f3) + "\n";
		}
		startIndex += mesh->GetVerticesAmount();
#pragma endregion
		of.write(data.c_str(), data.size());
		of.flush();
	}
}

void SorghumLayer::ExportAllSorghumsModel(const std::string& filename) {
	std::ofstream of;
	of.open(filename, std::ofstream::out | std::ofstream::trunc);
	if (of.is_open()) {
		std::string start = "#Sorghum field, by Bosheng Li";
		start += "\n";
		of.write(start.c_str(), start.size());
		of.flush();
		auto scene = GetScene();
		unsigned startIndex = 1;
		std::vector<Entity> sorghums;
		for (const auto& plant : sorghums) {
			ExportSorghum(plant, of, startIndex);
		}
		of.close();
		EVOENGINE_LOG("Sorghums saved as " + filename);
	}
	else {
		EVOENGINE_ERROR("Can't open file!");
	}
}


#ifdef BUILD_WITH_RAYTRACER
void SorghumLayer::CalculateIlluminationFrameByFrame() {
	auto scene = GetScene();
	const auto* owners = scene->UnsafeGetPrivateComponentOwnersList<
		TriangleIlluminationEstimator>();
	if (!owners)
		return;
	m_processingEntities.clear();

	m_processingEntities.insert(m_processingEntities.begin(), owners->begin(),
		owners->end());
	m_processingIndex = m_processingEntities.size();
	m_processing = true;
}
void SorghumLayer::CalculateIllumination() {
	auto scene = GetScene();
	const auto* owners = scene->UnsafeGetPrivateComponentOwnersList<
		TriangleIlluminationEstimator>();
	if (!owners)
		return;
	m_processingEntities.clear();

	m_processingEntities.insert(m_processingEntities.begin(), owners->begin(),
		owners->end());
	m_processingIndex = m_processingEntities.size();
	while (m_processing) {
		m_processingIndex--;
		if (m_processingIndex == -1) {
			m_processing = false;
		}
		else {
			const float timer = Times::Now();
			auto estimator =
				scene
				->GetOrSetPrivateComponent<TriangleIlluminationEstimator>(
					m_processingEntities[m_processingIndex])
				.lock();
			estimator->PrepareLightProbeGroup();
			estimator->SampleLightProbeGroup(m_rayProperties, m_seed,
				m_pushDistance);
		}
	}
}
#endif
void SorghumLayer::Update() {
	auto scene = GetScene();
#ifdef BUILD_WITH_RAYTRACER
	if (m_processing) {
		m_processingIndex--;
		if (m_processingIndex == -1) {
			m_processing = false;
		}
		else {
			const float timer = Times::Now();
			auto estimator =
				scene
				->GetOrSetPrivateComponent<TriangleIlluminationEstimator>(
					m_processingEntities[m_processingIndex])
				.lock();
			estimator->PrepareLightProbeGroup();
			estimator->SampleLightProbeGroup(m_rayProperties, m_seed,
				m_pushDistance);
			m_perPlantCalculationTime = Times::Now() - timer;
		}
}
#endif
}

void SorghumLayer::LateUpdate() {
	if (m_autoRefreshSorghums) {
		auto scene = GetScene();
		std::vector<Entity> plants;
		for (auto& plant : plants) {
			
		}
	}
}
