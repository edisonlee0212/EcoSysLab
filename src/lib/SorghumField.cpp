//
// Created by lllll on 9/16/2021.
//

#include "SorghumField.hpp"

#include "SorghumData.hpp"
#include "SorghumLayer.hpp"
#include "SorghumDescriptor.hpp"
#include "Scene.hpp"
#include "EditorLayer.hpp"
#include "TransformGraph.hpp"

using namespace EcoSysLab;
void RectangularSorghumFieldPattern::GenerateField(
	std::vector<glm::mat4>& matricesList) {
	matricesList.clear();
	for (int xi = 0; xi < m_size.x; xi++) {
		for (int yi = 0; yi < m_size.y; yi++) {
			auto position =
				glm::gaussRand(glm::vec3(0.0f), glm::vec3(m_distanceVariance.x, 0.0f,
					m_distanceVariance.y)) +
				glm::vec3(xi * m_distance.x, 0.0f, yi * m_distance.y);
			auto rotation = glm::quat(glm::radians(
				glm::vec3(glm::gaussRand(glm::vec3(0.0f), m_rotationVariance))));
			matricesList.emplace_back(glm::translate(position) *
				glm::mat4_cast(rotation) *
				glm::scale(glm::vec3(1.0f)));
		}
	}
}

void SorghumField::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) {

	ImGui::DragInt("Size limit", &m_sizeLimit, 1, 0, 10000);
	ImGui::DragFloat("Sorghum size", &m_sorghumSize, 0.01f, 0, 10);
	if (ImGui::Button("Refresh matrices")) {
		GenerateMatrices();
	}
	if (ImGui::Button("Instantiate")) {
		InstantiateField();
	}

	ImGui::Text("Matrices count: %d", (int)m_matrices.size());
}
void SorghumField::Serialize(YAML::Emitter& out) {
	out << YAML::Key << "m_sizeLimit" << YAML::Value << m_sizeLimit;
	out << YAML::Key << "m_sorghumSize" << YAML::Value << m_sorghumSize;

	out << YAML::Key << "m_matrices" << YAML::Value << YAML::BeginSeq;
	for (auto& i : m_matrices) {
		out << YAML::BeginMap;
		i.first.Save("SPD", out);
		out << YAML::Key << "Transform" << YAML::Value << i.second;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
}
void SorghumField::Deserialize(const YAML::Node& in) {
	if (in["m_sizeLimit"])
		m_sizeLimit = in["m_sizeLimit"].as<int>();
	if (in["m_sorghumSize"])
		m_sorghumSize = in["m_sorghumSize"].as<float>();

	m_matrices.clear();
	if (in["m_matrices"]) {
		for (const auto& i : in["m_matrices"]) {
			AssetRef spd;
			spd.Load("SPD", i);
			m_matrices.emplace_back(spd, i["Transform"].as<glm::mat4>());
		}
	}
}
void SorghumField::CollectAssetRef(std::vector<AssetRef>& list) {
	for (auto& i : m_matrices) {
		list.push_back(i.first);
	}
}
Entity SorghumField::InstantiateField() {
	if (m_matrices.empty())
		GenerateMatrices();
	if (m_matrices.empty()) {
		EVOENGINE_ERROR("No matrices generated!");
		return {};
	}


	auto sorghumLayer = Application::GetLayer<SorghumLayer>();
	auto scene = sorghumLayer->GetScene();
	if (sorghumLayer) {
		auto fieldAsset = std::dynamic_pointer_cast<SorghumField>(GetSelf());
		auto field = scene->CreateEntity("Field");
		// Create sorghums here.
		int size = 0;
		for (auto& newSorghum : fieldAsset->m_matrices) {
			Entity sorghumEntity = sorghumLayer->CreateSorghum();
			auto sorghumTransform = scene->GetDataComponent<Transform>(sorghumEntity);
			sorghumTransform.m_value = newSorghum.second;
			sorghumTransform.SetScale(glm::vec3(m_sorghumSize));
			scene->SetDataComponent(sorghumEntity, sorghumTransform);
			auto sorghumData =
				scene->GetOrSetPrivateComponent<SorghumData>(sorghumEntity).lock();
			sorghumData->m_mode = (int)SorghumMode::SorghumDescriptor;
			sorghumData->m_descriptor = newSorghum.first;
			sorghumData->m_seed = size;
			sorghumData->SetTime(1.0f);
			scene->SetParent(sorghumEntity, field);
			size++;
			if (size >= m_sizeLimit)
				break;
		}

		Application::GetLayer<SorghumLayer>()->GenerateMeshForAllSorghums();

		TransformGraph::CalculateTransformGraphForDescendents(scene,
			field);
		return field;
	}
	else {
		EVOENGINE_ERROR("No sorghum layer!");
		return {};
	}
}

