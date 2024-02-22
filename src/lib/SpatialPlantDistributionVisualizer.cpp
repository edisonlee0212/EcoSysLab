#include "SpatialPlantDistributionVisualizer.hpp"

using namespace EcoSysLab;

void SpatialPlantDistributionVisualizer::OnInspectSpatialPlantDistributionFunction(
	const SpatialPlantDistribution& spatialPlantDistribution,
	const std::function<void(glm::vec2 position)>& func, const std::function<void(ImVec2 origin, float zoomFactor, ImDrawList*)>& drawFunc)
{
	static auto scrolling = glm::vec2(0.0f);
	static float zoomFactor = 0.1f;
	ImGui::Text(("Plant count: " + std::to_string(spatialPlantDistribution.m_plants.size() - spatialPlantDistribution.m_recycledPlants.size()) +
		" | Simulation time: " + std::to_string(spatialPlantDistribution.m_simulationTime)).c_str());
	if (ImGui::Button("Recenter")) {
		scrolling = glm::vec2(0.0f);
	}
	ImGui::SameLine();
	ImGui::DragFloat("Zoom", &zoomFactor, zoomFactor / 100.0f, 0.001f, 10.0f);
	zoomFactor = glm::clamp(zoomFactor, 0.01f, 1000.0f);
	const ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImVec2 canvasP0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvasSz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	if (canvasSz.x < 300.0f) canvasSz.x = 300.0f;
	if (canvasSz.y < 300.0f) canvasSz.y = 300.0f;
	const ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);
	const ImVec2 origin(canvasP0.x + canvasSz.x / 2.0f + scrolling.x,
		canvasP0.y + canvasSz.y / 2.0f + scrolling.y); // Lock scrolled origin
	const ImVec2 mousePosInCanvas((io.MousePos.x - origin.x) / zoomFactor,
		(io.MousePos.y - origin.y) / zoomFactor);

	// Draw border and background color
	drawList->AddRectFilled(canvasP0, canvasP1, IM_COL32(50, 50, 50, 255));
	drawList->AddRect(canvasP0, canvasP1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("canvas", canvasSz,
		ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	const bool isMouseHovered = ImGui::IsItemHovered(); // Hovered
	const bool isMouseActive = ImGui::IsItemActive();   // Held

	// Pan (we use a zero mouse threshold when there's no context menu)
	// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
	const float mouseThresholdForPan = -1.0f;
	if (isMouseActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouseThresholdForPan)) {
		scrolling.x += io.MouseDelta.x;
		scrolling.y += io.MouseDelta.y;
	}
	// Context menu (under default mouse threshold)
	const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
	if (dragDelta.x == 0.0f && dragDelta.y == 0.0f)
		ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
	if (ImGui::BeginPopup("context")) {

		ImGui::EndPopup();
	}

	// Draw profile + all lines in the canvas
	drawList->PushClipRect(canvasP0, canvasP1, true);
	if (isMouseHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		func(glm::vec2(mousePosInCanvas.x, mousePosInCanvas.y));
	}
	const size_t mod = spatialPlantDistribution.m_plants.size() / 15000;
	int index = 0;
	for (const auto& plant : spatialPlantDistribution.m_plants) {
		if (plant.m_recycled) continue;
		index++;
		if (mod > 1 && index % mod != 0) continue;
		const auto& pointPosition = plant.m_position;
		const auto& pointColor = spatialPlantDistribution.m_spatialPlantParameters[plant.m_parameterHandle].m_color;
		const auto canvasPosition = 
			ImVec2(origin.x + pointPosition.x * zoomFactor, origin.y + pointPosition.y * zoomFactor);
		drawList->AddCircle(canvasPosition, zoomFactor * plant.m_radius,
			IM_COL32(255.0f * pointColor.x, 255.0f * pointColor.y, 255.0f * pointColor.z, 255.0f));
	}
	drawList->AddCircle(origin,
		glm::clamp(zoomFactor, 1.0f, 100.0f),
		IM_COL32(255,
			0,
			0, 255));
	drawFunc(origin, zoomFactor, drawList);
	drawList->PopClipRect();
}

void SpatialPlantDistributionVisualizer::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer)
{
	if(ImGui::TreeNode("Parameters"))
	{
		static SpatialPlantParameter temp{};
		ImGui::DragFloat("Final size", &temp.m_w, 1.0f, 1.0f, 100.0f);
		ImGui::DragFloat("Growth rate", &temp.m_k, 0.01f, 0.01f, 1.0f);
		ImGui::ColorEdit4("Color", &temp.m_color.x);
		if(ImGui::Button("Push"))
		{
			m_distribution.m_spatialPlantParameters.emplace_back(temp);
		}
		int index = 0;
		for(const auto& parameter : m_distribution.m_spatialPlantParameters)
		{
			ImGui::ColorButton(("No. " + std::to_string(index) 
				+ ", FS[" + std::to_string(parameter.m_w) + "]"
				+ ", GR[" + std::to_string(parameter.m_k) + "]").c_str(), ImVec4(parameter.m_color.x, parameter.m_color.y, parameter.m_color.z, parameter.m_color.w));
			index++;
		}
		ImGui::TreePop();
	}
	
	if(ImGui::TreeNode("Generate Plants..."))
	{
		static int plantSize = 10;
		static float initialRadius = 1.f;
		static float diskSize = 5.0f;
		static int parameterHandle = 0;
		ImGui::DragInt("Plant size", &plantSize, 10, 10, 1000);
		ImGui::DragFloat("Initial radius", &initialRadius, 1.f, 1.f, 10.f);
		ImGui::DragFloat("Disk size", &diskSize, 1.f, 1.f, 1000.f);
		ImGui::DragInt("Parameter Index", &parameterHandle, 1, 0, static_cast<int>(m_distribution.m_spatialPlantParameters.size()) - 1);
		parameterHandle = glm::clamp(parameterHandle, 0, static_cast<int>(m_distribution.m_spatialPlantParameters.size()) - 1);
		if (ImGui::Button("Generate")) {
			for (int i = 0; i < plantSize; i++)
			{
				m_distribution.AddPlant(parameterHandle, initialRadius, glm::diskRand(diskSize));
			}
		}
		ImGui::TreePop();
	}

	ImGui::Checkbox("Simulate", &m_simulate);

	const std::string tag = "Spatial Plant Scene";
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Appearing);
	if (ImGui::Begin(tag.c_str()))
	{
		OnInspectSpatialPlantDistributionFunction(m_distribution, [&](glm::vec2 position)
		{
			
		}, [&](ImVec2 origin, float zoomFactor, ImDrawList*)
		{
			
		});
	}
	ImGui::End();
}

void SpatialPlantDistributionVisualizer::FixedUpdate()
{
	if (m_simulate)
	{
		m_distribution.Simulate();
	}
}

void SpatialPlantDistributionVisualizer::OnCreate()
{
	m_distribution = {};
	m_distribution.m_spatialPlantParameters.emplace_back();
}
