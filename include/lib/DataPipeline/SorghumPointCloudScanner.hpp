#pragma once

#include "PointCloudScannerUtils.hpp"
using namespace EvoEngine;
namespace EcoSysLab
{
    struct SorghumPointCloudPointSettings {
        float m_variance = 0.015f;
        float m_ballRandRadius = 0.01f;

        bool m_typeIndex = true;
        bool m_instanceIndex = true;
        bool m_leafIndex = true;

        float m_boundingBoxLimit = 1.f;

        void OnInspect();
        void Save(const std::string& name, YAML::Emitter& out) const;
        void Load(const std::string& name, const YAML::Node& in);
    };

    class SorghumPointCloudGridCaptureSettings : public PointCloudCaptureSettings
    {
    public:
        float m_boundingBoxSize = 3.;

        glm::ivec2 m_gridSize = { 5, 5 };
        float m_gridDistance = 1.25f;
        float m_step = 0.01f;
        int m_droneSample = 512;
        float m_droneHeight = 2.5f;
        void OnInspect() override;
        void GenerateSamples(std::vector<PointCloudSample>& pointCloudSamples) override;
        bool SampleFilter(const PointCloudSample& sample) override;
    };

    class SorghumGantryCaptureSettings : public PointCloudCaptureSettings
    {
    public:
        float m_boundingBoxSize = 3.;

        glm::ivec2 m_gridSize = { 5, 5 };
        glm::vec2 m_gridDistance = { 0.75f, 0.75f };
        glm::vec2 m_step = { 0.0075f,0.0075f };
        float m_sampleHeight = 2.5f;
        float m_scannerAngle = 30.f;
        void OnInspect() override;
        void GenerateSamples(std::vector<PointCloudSample>& pointCloudSamples) override;
        bool SampleFilter(const PointCloudSample& sample) override;
    };

    class SorghumPointCloudCaptureSettings : public PointCloudCaptureSettings
    {
    public:
        void OnInspect() override;
        void GenerateSamples(std::vector<PointCloudSample>& pointCloudSamples) override;
        bool SampleFilter(const PointCloudSample& sample) override;
    };

	class SorghumPointCloudScanner : public IPrivateComponent
	{
	public:
        SorghumPointCloudPointSettings m_sorghumPointCloudPointSettings {};
        void Capture(const std::filesystem::path& savePath, const std::shared_ptr<PointCloudCaptureSettings>& captureSettings) const;
        void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;

        void OnDestroy() override;

        void Serialize(YAML::Emitter& out) override;
        void Deserialize(const YAML::Node& in) override;
	};
}