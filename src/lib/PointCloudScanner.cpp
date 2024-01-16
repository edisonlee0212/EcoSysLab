#include "PointCloudScanner.hpp"

#ifdef BUILD_WITH_RAYTRACER
#include <RayTracerLayer.hpp>
#include <CUDAModule.hpp>
#include <RayTracer.hpp>
#endif
#include "Tinyply.hpp"
using namespace tinyply;
#include "EcoSysLabLayer.hpp"
using namespace EcoSysLab;


void PointCloudPointSettings::OnInspect()
{
	ImGui::DragFloat("Point variance", &m_variance, 0.01f);
	ImGui::DragFloat("Point uniform random radius", &m_ballRandRadius, 0.01f);
	ImGui::DragFloat("Bounding box offset", &m_boundingBoxLimit, 0.01f);
	ImGui::Checkbox("Type Index", &m_typeIndex);
	ImGui::Checkbox("Instance Index", &m_instanceIndex);
	ImGui::Checkbox("Branch Index", &m_branchIndex);
	ImGui::Checkbox("Tree Part Index", &m_treePartIndex);
	ImGui::Checkbox("Line Index", &m_lineIndex);
	ImGui::Checkbox("Internode Index", &m_internodeIndex);
}

void PointCloudPointSettings::Serialize(const std::string& name, YAML::Emitter& out) const
{
	out << YAML::Key << name << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "m_variance" << YAML::Value << m_variance;
	out << YAML::Key << "m_ballRandRadius" << YAML::Value << m_ballRandRadius;
	out << YAML::Key << "m_typeIndex" << YAML::Value << m_typeIndex;
	out << YAML::Key << "m_instanceIndex" << YAML::Value << m_instanceIndex;
	out << YAML::Key << "m_branchIndex" << YAML::Value << m_branchIndex;
	out << YAML::Key << "m_treePartIndex" << YAML::Value << m_treePartIndex;
	out << YAML::Key << "m_lineIndex" << YAML::Value << m_lineIndex;
	out << YAML::Key << "m_internodeIndex" << YAML::Value << m_internodeIndex;
	out << YAML::Key << "m_boundingBoxLimit" << YAML::Value << m_boundingBoxLimit;
	out << YAML::EndMap;
}

void PointCloudPointSettings::Deserialize(const std::string& name, const YAML::Node& in)
{
	if (in[name]) {
		auto& cd = in[name];
		if (cd["m_variance"]) m_variance = cd["m_variance"].as<float>();
		if (cd["m_ballRandRadius"]) m_ballRandRadius = cd["m_ballRandRadius"].as<float>();
		if (cd["m_typeIndex"]) m_typeIndex = cd["m_typeIndex"].as<bool>();
		if (cd["m_instanceIndex"]) m_instanceIndex = cd["m_instanceIndex"].as<bool>();
		if (cd["m_branchIndex"]) m_branchIndex = cd["m_branchIndex"].as<bool>();
		if (cd["m_treePartIndex"]) m_treePartIndex = cd["m_treePartIndex"].as<bool>();
		if (cd["m_lineIndex"]) m_lineIndex = cd["m_lineIndex"].as<bool>();
		if (cd["m_internodeIndex"]) m_internodeIndex = cd["m_internodeIndex"].as<bool>();
		if (cd["m_boundingBoxLimit"]) m_boundingBoxLimit = cd["m_boundingBoxLimit"].as<float>();
	}
}

void PointCloudCircularCaptureSettings::OnInspect()
{
	ImGui::DragFloat("Distance to focus point", &m_distance, 0.01f);
	ImGui::DragFloat("Height to ground", &m_height, 0.01f);
	ImGui::Separator();
	ImGui::Text("Rotation:");
	ImGui::DragInt3("Pitch Angle Start/Step/End", &m_pitchAngleStart, 1);
	ImGui::DragInt3("Turn Angle Start/Step/End", &m_turnAngleStart, 1);
	ImGui::Separator();
	ImGui::Text("Camera Settings:");
	ImGui::DragFloat("FOV", &m_fov);
	ImGui::DragInt("Resolution", &m_resolution);
	ImGui::DragFloat("Max Depth", &m_cameraDepthMax);

}

void PointCloudCircularCaptureSettings::Serialize(const std::string& name, YAML::Emitter& out) const
{
	out << YAML::Key << name << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "m_pitchAngleStart" << YAML::Value << m_pitchAngleStart;
	out << YAML::Key << "m_pitchAngleStep" << YAML::Value << m_pitchAngleStep;
	out << YAML::Key << "m_pitchAngleEnd" << YAML::Value << m_pitchAngleEnd;
	out << YAML::Key << "m_turnAngleStart" << YAML::Value << m_turnAngleStart;
	out << YAML::Key << "m_turnAngleStep" << YAML::Value << m_turnAngleStep;
	out << YAML::Key << "m_turnAngleEnd" << YAML::Value << m_turnAngleEnd;
	out << YAML::Key << "m_distance" << YAML::Value << m_distance;
	out << YAML::Key << "m_height" << YAML::Value << m_height;
	out << YAML::Key << "m_fov" << YAML::Value << m_fov;
	out << YAML::Key << "m_resolution" << YAML::Value << m_resolution;
	out << YAML::Key << "m_cameraDepthMax" << YAML::Value << m_cameraDepthMax;
	out << YAML::EndMap;
}

void PointCloudCircularCaptureSettings::Deserialize(const std::string& name, const YAML::Node& in)
{
	if (in[name]) {
		auto& cd = in[name];

		if (cd["m_pitchAngleStart"]) m_pitchAngleStart = cd["m_pitchAngleStart"].as<int>();
		if (cd["m_pitchAngleStep"]) m_pitchAngleStep = cd["m_pitchAngleStep"].as<int>();
		if (cd["m_pitchAngleEnd"]) m_pitchAngleEnd = cd["m_pitchAngleEnd"].as<int>();
		if (cd["m_turnAngleStart"]) m_turnAngleStart = cd["m_turnAngleStart"].as<int>();
		if (cd["m_turnAngleStep"]) m_turnAngleStep = cd["m_turnAngleStep"].as<int>();
		if (cd["m_turnAngleEnd"]) m_turnAngleEnd = cd["m_turnAngleEnd"].as<int>();
		if (cd["m_distance"]) m_distance = cd["m_distance"].as<float>();
		if (cd["m_height"]) m_height = cd["m_height"].as<float>();
		if (cd["m_fov"]) m_fov = cd["m_fov"].as<float>();
		if (cd["m_resolution"]) m_resolution = cd["m_resolution"].as<int>();
		if (cd["m_cameraDepthMax"]) m_cameraDepthMax = cd["m_cameraDepthMax"].as<float>();

	}
}

GlobalTransform PointCloudCircularCaptureSettings::GetTransform(const glm::vec2& focusPoint,
	const float turnAngle, const float pitchAngle) const
{
	GlobalTransform cameraGlobalTransform;
	const glm::vec3 cameraPosition =
		glm::vec3(glm::sin(glm::radians(turnAngle)) * m_distance,
			m_height,
			glm::cos(glm::radians(turnAngle)) * m_distance);
	const glm::vec3 cameraDirection =
		glm::vec3(glm::sin(glm::radians(turnAngle)) * m_distance,
			m_distance * glm::sin(glm::radians(pitchAngle)),
			glm::cos(glm::radians(turnAngle)) * m_distance);
	cameraGlobalTransform.SetPosition(cameraPosition + glm::vec3(focusPoint.x, 0, focusPoint.y));
	cameraGlobalTransform.SetRotation(glm::quatLookAt(glm::normalize(-cameraDirection), glm::vec3(0, 1, 0)));
	return cameraGlobalTransform;
}

void PointCloudCircularCaptureSettings::GenerateSamples(std::vector<PointCloudSample>& pointCloudSamples)
{
	int counter = 0;
	for (int turnAngle = m_turnAngleStart;
		turnAngle < m_turnAngleEnd; turnAngle += m_turnAngleStep) {
		for (int pitchAngle = m_pitchAngleStart;
			pitchAngle < m_pitchAngleEnd; pitchAngle += m_pitchAngleStep) {
			pointCloudSamples.resize((counter + 1) * m_resolution * m_resolution);
			auto scannerGlobalTransform = GetTransform(glm::vec2(m_focusPoint.x, m_focusPoint.y), turnAngle, pitchAngle);
			auto front = scannerGlobalTransform.GetRotation() * glm::vec3(0, 0, -1);
			auto up = scannerGlobalTransform.GetRotation() * glm::vec3(0, 1, 0);
			auto left = scannerGlobalTransform.GetRotation() * glm::vec3(1, 0, 0);
			auto position = scannerGlobalTransform.GetPosition();
			std::vector<std::shared_future<void>> results;
			Jobs::ParallelFor(
				m_resolution * m_resolution,
				[&](const unsigned i) {
					const float x = i % m_resolution;
					const float y = i / m_resolution;
					const float xAngle = (x - m_resolution / 2.0f + glm::linearRand(-0.5f, 0.5f)) /
						static_cast<float>(m_resolution) * m_fov /
						2.0f;
					const float yAngle = (y - m_resolution / 2.0f + glm::linearRand(-0.5f, 0.5f)) /
						static_cast<float>(m_resolution) * m_fov /
						2.0f;
					auto& sample = pointCloudSamples[
						counter * m_resolution * m_resolution +
							i];
					sample.m_direction = glm::normalize(glm::rotate(glm::rotate(front, glm::radians(xAngle), left),
						glm::radians(yAngle), up));
					sample.m_start = position;
				});
			counter++;
		}
	}
}

void PointCloudGridCaptureSettings::OnInspect()
{
	ImGui::DragInt2("Grid size", &m_gridSize.x, 1, 0, 100);
	ImGui::DragFloat("Grid distance", &m_gridDistance, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Step", &m_step, 0.01f, 0.0f, 0.5f);
	ImGui::DragInt("Sample", &m_backpackSample, 1, 1, INT_MAX);
}

void PointCloudGridCaptureSettings::GenerateSamples(std::vector<PointCloudSample>& pointCloudSamples)
{
	const glm::vec2 startPoint = glm::vec2((static_cast<float>(m_gridSize.x) * 0.5f - 0.5f) * m_gridDistance, (static_cast<float>(m_gridSize.y) * 0.5f - 0.5f) * m_gridDistance);

	const int yStepSize = m_gridSize.y * m_gridDistance / m_step;
	const int xStepSize = m_gridSize.x * m_gridDistance / m_step;

	pointCloudSamples.resize((m_gridSize.x * yStepSize + m_gridSize.y * xStepSize) * (m_backpackSample + m_droneSample));
	unsigned startIndex = 0;
	for (int i = 0; i < m_gridSize.x; i++) {
		float x = i * m_gridDistance;
		for (int step = 0; step < yStepSize; step++)
		{
			float z = step * m_step;
			const glm::vec3 center = glm::vec3{ x, m_backpackHeight, z } - glm::vec3(startPoint.x, 0, startPoint.y);
			Jobs::ParallelFor(m_backpackSample, [&](unsigned sampleIndex)
				{
					auto& sample = pointCloudSamples[m_backpackSample * (i * yStepSize + step) + sampleIndex];
					sample.m_direction = glm::sphericalRand(1.0f);
					if (glm::linearRand(0.0f, 1.0f) > 0.3f)
					{
						sample.m_direction.y = glm::abs(sample.m_direction.y);
					}
					else
					{
						sample.m_direction.y = -glm::abs(sample.m_direction.y);
					}
					sample.m_start = center;
				}
			);
		}
	}

	startIndex += m_gridSize.x * yStepSize * m_backpackSample;
	for (int i = 0; i < m_gridSize.y; i++) {
		float z = i * m_gridDistance;
		for (int step = 0; step < xStepSize; step++)
		{
			float x = step * m_step;
			const glm::vec3 center = glm::vec3{ x, m_backpackHeight, z } - glm::vec3(startPoint.x, 0, startPoint.y);
			Jobs::ParallelFor(m_backpackSample, [&](unsigned sampleIndex)
				{
					auto& sample = pointCloudSamples[startIndex + m_backpackSample * (i * xStepSize + step) + sampleIndex];
					sample.m_direction = glm::sphericalRand(1.0f);
					if (glm::linearRand(0.0f, 1.0f) > 0.3f)
					{
						sample.m_direction.y = glm::abs(sample.m_direction.y);
					}
					else
					{
						sample.m_direction.y = -glm::abs(sample.m_direction.y);
					}
					sample.m_start = center;
				}
			);
		}
	}

	startIndex += m_gridSize.y * xStepSize * m_backpackSample;
	for (int i = 0; i < m_gridSize.x; i++) {
		float x = i * m_gridDistance;
		for (int step = 0; step < yStepSize; step++)
		{
			float z = step * m_step;
			const glm::vec3 center = glm::vec3{ x, m_droneHeight, z } - glm::vec3(startPoint.x, 0, startPoint.y);
			Jobs::ParallelFor(m_droneSample, [&](unsigned sampleIndex)
				{
					auto& sample = pointCloudSamples[m_droneSample * (i * yStepSize + step) + sampleIndex];
					sample.m_direction = glm::sphericalRand(1.0f);
					sample.m_direction.y = -glm::abs(sample.m_direction.y);
					sample.m_start = center;
				}
			);
		}
	}

	startIndex += m_gridSize.x * yStepSize * m_droneSample;
	for (int i = 0; i < m_gridSize.y; i++) {
		float z = i * m_gridDistance;
		for (int step = 0; step < xStepSize; step++)
		{
			float x = step * m_step;
			const glm::vec3 center = glm::vec3{ x, m_droneHeight, z } - glm::vec3(startPoint.x, 0, startPoint.y);
			Jobs::ParallelFor(m_droneSample, [&](unsigned sampleIndex)
				{
					auto& sample = pointCloudSamples[startIndex + m_droneSample * (i * xStepSize + step) + sampleIndex];
					sample.m_direction = glm::sphericalRand(1.0f);
					sample.m_direction.y = -glm::abs(sample.m_direction.y);
					sample.m_start = center;
				}
			);
		}
	}
}

bool PointCloudGridCaptureSettings::SampleFilter(const PointCloudSample& sample)
{
	return glm::abs(sample.m_hitInfo.m_position.x) < m_boundingBoxSize && glm::abs(sample.m_hitInfo.m_position.z) < m_boundingBoxSize;
}

void PointCloudScanner::Capture(const std::filesystem::path& savePath,
	const std::shared_ptr<PointCloudCaptureSettings>& captureSettings) const
{
#ifdef BUILD_WITH_RAYTRACER
	std::vector<PointCloudSample> pcSamples;
	captureSettings->GenerateSamples(pcSamples);
	CudaModule::SamplePointCloud(
		Application::GetLayer<RayTracerLayer>()->m_environmentProperties,
		pcSamples);

	std::vector<glm::vec3> points;

	for (const auto& sample : pcSamples) {
		if (!sample.m_hit) continue;
		if (!captureSettings->SampleFilter(sample)) continue;
		auto ballRand = glm::vec3(0.0f);
		if (m_pointSettings.m_ballRandRadius > 0.0f) {
			ballRand = glm::ballRand(m_pointSettings.m_ballRandRadius);
		}
		const auto distance = glm::distance(sample.m_hitInfo.m_position, sample.m_start);
		points.emplace_back(
			sample.m_hitInfo.m_position +
			distance * glm::vec3(glm::gaussRand(0.0f, m_pointSettings.m_variance),
				glm::gaussRand(0.0f, m_pointSettings.m_variance),
				glm::gaussRand(0.0f, m_pointSettings.m_variance))
			+ ballRand);
	}
	std::filebuf fb_binary;
	fb_binary.open(savePath.string(), std::ios::out | std::ios::binary);
	std::ostream outstream_binary(&fb_binary);
	if (outstream_binary.fail())
		throw std::runtime_error("failed to open " + savePath.string());
	
	PlyFile cube_file;
	cube_file.add_properties_to_element(
		"vertex", { "x", "y", "z" }, Type::FLOAT32, points.size(),
		reinterpret_cast<uint8_t*>(points.data()), Type::INVALID, 0);
	// Write a binary file
	cube_file.write(outstream_binary, true);
#endif
}

void PointCloudScanner::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer)
{
	if (ImGui::TreeNodeEx("Circular Capture")) {
		static std::shared_ptr<PointCloudCircularCaptureSettings> captureSettings = std::make_shared<PointCloudCircularCaptureSettings>();
		captureSettings->OnInspect();
		FileUtils::SaveFile("Capture", "Point Cloud", { ".ply" }, [&](const std::filesystem::path& path) {
			Capture(path, captureSettings);
			}, false);
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Grid Capture")) {
		static std::shared_ptr<PointCloudGridCaptureSettings> captureSettings = std::make_shared<PointCloudGridCaptureSettings>();
		captureSettings->OnInspect();
		FileUtils::SaveFile("Capture", "Point Cloud", { ".ply" }, [&](const std::filesystem::path& path) {
			Capture(path, captureSettings);
			}, false);
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Point settings")) {
		m_pointSettings.OnInspect();
		ImGui::TreePop();
	}
}

void PointCloudScanner::OnDestroy()
{
	m_pointSettings = {};
}

void PointCloudScanner::Serialize(YAML::Emitter& out)
{
	m_pointSettings.Serialize("m_pointSettings", out);
}

void PointCloudScanner::Deserialize(const YAML::Node& in)
{
	m_pointSettings.Deserialize("m_pointSettings", in);
}