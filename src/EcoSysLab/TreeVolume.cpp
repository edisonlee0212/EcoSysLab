#include "TreeVolume.hpp"
using namespace EcoSysLab;
int TreeSphericalVolume::GetSectorIndex(const glm::vec3& position) const
{
	const float layerAngle = 180.0f / m_layerAmount;
	const float sectorAngle = 360.0f / m_sectorAmount;

	const auto v = position - m_center;
	if (glm::abs(v.x + v.z) < 0.000001f) return 0;
	const auto dot = glm::dot(glm::normalize(v), glm::vec3(0, 1, 0));
	const auto rd = glm::degrees(glm::acos(dot));
	const int layerIndex = rd / layerAngle;
	const int sectorIndex = (glm::degrees(glm::atan(v.x, v.z)) + 180.0f) / sectorAngle;
	return glm::clamp(layerIndex * m_sectorAmount + sectorIndex, 0, m_layerAmount * m_sectorAmount - 1);
}

void TreeSphericalVolume::Clear()
{
	m_center = glm::vec3(0.0f);
	m_distances.resize(m_layerAmount * m_sectorAmount);
	for (auto& distance : m_distances)
	{
		distance = -1;
	}
	m_hasData = false;
}

void TreeSphericalVolume::TipPosition(int layerIndex, int sectorIndex, glm::vec3& position) const
{
	position = m_center;
	const float layerAngle = glm::radians(180.0f / m_layerAmount * (layerIndex + 0.5f));
	const float sectorAngle = glm::radians(360.0f / m_sectorAmount * (sectorIndex + 0.5f));

	const auto& distance = m_distances[layerIndex * m_sectorAmount + sectorIndex];
	if (distance < 0.0f) return;
	position.y += glm::cos(layerAngle) * distance;
	position.x -= glm::sin(layerAngle) * distance * glm::cos(sectorAngle + glm::radians(270.0f));
	position.z += glm::sin(layerAngle) * distance * glm::sin(sectorAngle + glm::radians(270.0f));
}

void TreeSphericalVolume::Smooth()
{
	auto copy = m_distances;
	for (int i = 0; i < m_layerAmount; i++)
	{
		for (int j = 0; j < m_sectorAmount; j++)
		{
			int count = 0;
			float sum = 0.0f;
			if (m_distances[i * m_sectorAmount + j] != -1) continue;
			if (i > 0) {
				if (m_distances[(i - 1) * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount] != -1)
				{
					count++;
					sum += m_distances[(i - 1) * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount];
				}
				if (m_distances[(i - 1) * m_sectorAmount + j] != -1)
				{
					count++;
					sum += m_distances[(i - 1) * m_sectorAmount + j];
				}
				if (m_distances[(i - 1) * m_sectorAmount + (j + 1) % m_sectorAmount] != -1)
				{
					count++;
					sum += m_distances[(i - 1) * m_sectorAmount + (j + 1) % m_sectorAmount];
				}
			}
			if (m_distances[i * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount] != -1)
			{
				count++;
				sum += m_distances[i * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount];
			}
			if (m_distances[i * m_sectorAmount + (j + 1) % m_sectorAmount] != -1)
			{
				count++;
				sum += m_distances[i * m_sectorAmount + (j + 1) % m_sectorAmount];
			}
			if (i < m_layerAmount - 1) {
				if (m_distances[(i + 1) * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount] != -1)
				{
					count++;
					sum += m_distances[(i + 1) * m_sectorAmount + (j + m_sectorAmount - 1) % m_sectorAmount];
				}
				if (m_distances[(i + 1) * m_sectorAmount + j] != -1)
				{
					count++;
					sum += m_distances[(i + 1) * m_sectorAmount + j];
				}
				if (m_distances[(i + 1) * m_sectorAmount + (j + 1) % m_sectorAmount] != -1)
				{
					count++;
					sum += m_distances[(i + 1) * m_sectorAmount + (j + 1) % m_sectorAmount];
				}
			}
			if (count != 0) copy[i * m_sectorAmount + j] = sum / count;
			else
			{
				copy[i * m_sectorAmount + j] = 0;
			}
		}
	}
	m_distances = copy;
}

float TreeSphericalVolume::IlluminationEstimation(const glm::vec3& position,
	const IlluminationEstimationSettings& settings, glm::vec3& lightDirection)
{
	if (!m_hasData || m_distances.empty()) return 1.0f;
	const float layerAngle = 180.0f / settings.m_probeLayerAmount;
	const float sectorAngle = 360.0f / settings.m_probeSectorAmount;
	const auto maxSectorIndex = settings.m_probeLayerAmount * settings.m_probeSectorAmount - 1;
	m_probe.resize(settings.m_probeLayerAmount * settings.m_probeSectorAmount);
	for (auto& sector : m_probe) sector = { 0.0f, 0 };
	for (int i = 0; i < m_layerAmount; i++)
	{
		for (int j = 0; j < m_sectorAmount; j++)
		{
			glm::vec3 tipPosition;
			TipPosition(i, j, tipPosition);
			const float distance = glm::distance(position, tipPosition);
			auto diff = tipPosition - position;
			if (glm::abs(diff.x + diff.z) < 0.000001f) continue;
			const auto dot = glm::dot(glm::normalize(diff), glm::vec3(0, 1, 0));
			const auto rd = glm::degrees(glm::acos(dot));
			const int probeLayerIndex = rd / layerAngle;
			const int probeSectorIndex = (glm::degrees(glm::atan(diff.x, diff.z)) + 180.0f) / sectorAngle;
			auto& probeSector = m_probe[glm::clamp(probeLayerIndex * settings.m_probeSectorAmount + probeSectorIndex, 0, maxSectorIndex)];
			probeSector.first += distance;
			probeSector.second += 1;
		}
	}

	std::vector<float> probeAvg;
	probeAvg.resize(settings.m_probeLayerAmount * settings.m_probeSectorAmount);
	for (int i = 0; i < settings.m_probeLayerAmount; i++)
	{
		for (int j = 0; j < settings.m_probeSectorAmount; j++)
		{
			const auto index = i * settings.m_probeSectorAmount + j;
			const auto& probeSector = m_probe[index];
			if (probeSector.second == 0)
			{
				probeAvg[index] = -1;
			}
			else {
				probeAvg[index] = probeSector.first / probeSector.second;
			}
		}
	}
	{
		for (int i = 0; i < settings.m_probeLayerAmount; i++)
		{
			for (int j = 0; j < settings.m_probeSectorAmount; j++)
			{

				if (probeAvg[i * settings.m_probeSectorAmount + j] == -1) {
					int count = 0;
					float sum = 0.0f;

					if (i > 1) {
						/*
						if (probeAvg[i - 1][(j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount] != -1)
						{
							count++;
							sum += probeAvg[i - 1][(j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount];
						}
						*/
						if (probeAvg[(i - 1) * settings.m_probeSectorAmount + j] != -1)
						{
							count++;
							sum += probeAvg[(i - 1) * settings.m_probeSectorAmount + j];
						}
						/*
						if (probeAvg[i - 1][(j + 1) % settings.m_probeSectorAmount] != -1)
						{
							count++;
							sum += probeAvg[i - 1][(j + 1) % settings.m_probeSectorAmount];
						}
						*/
					}

					if (probeAvg[i * settings.m_probeSectorAmount + (j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount] != -1)
					{
						count++;
						sum += probeAvg[i * settings.m_probeSectorAmount + (j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount];
					}

					if (probeAvg[i * settings.m_probeSectorAmount + (j + 1) % settings.m_probeSectorAmount] != -1)
					{
						count++;
						sum += probeAvg[i * settings.m_probeSectorAmount + (j + 1) % settings.m_probeSectorAmount];
					}

					if (i < settings.m_probeLayerAmount - 1) {
						/*
						if (probeAvg[i + 1][(j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount] != -1)
						{
							count++;
							sum += probeAvg[i + 1][(j + settings.m_probeSectorAmount - 1) % settings.m_probeSectorAmount];
						}
						*/
						if (probeAvg[(i + 1) * settings.m_probeSectorAmount + j] != -1)
						{
							count++;
							sum += probeAvg[(i + 1) * settings.m_probeSectorAmount + j];
						}
						/*
						if (probeAvg[i + 1][(j + 1) % settings.m_probeSectorAmount] != -1)
						{
							count++;
							sum += probeAvg[i + 1][(j + 1) % settings.m_probeSectorAmount];
						}
						*/
					}
					if (count != 0) probeAvg[i * settings.m_probeSectorAmount + j] = sum / count;
					else probeAvg[i * settings.m_probeSectorAmount + j] = 0;
				}
			}
		}
	}

	float lightIntensity = 0.0f;
	float ratio = 1.0f;
	float ratioSum = 0.0f;
	auto direction = glm::vec3(0.0f);
	for (int i = 0; i < settings.m_probeLayerAmount; i++)
	{
		for (int j = 0; j < settings.m_probeSectorAmount; j++)
		{
			ratioSum += ratio;

			glm::vec3 lightDir = glm::vec3(0.0f);
			lightDir.y += glm::cos(glm::radians(i * layerAngle));
			lightDir.x -= glm::sin(glm::radians(i * layerAngle)) * glm::cos(glm::radians(j * sectorAngle + 270.0f));
			lightDir.z += glm::sin(glm::radians(i * layerAngle)) * glm::sin(glm::radians(j * sectorAngle + 270.0f));

			float sectorLightIntensity = ratio * (1.0f - glm::clamp(settings.m_occlusion * glm::pow(probeAvg[i * settings.m_probeSectorAmount + j], settings.m_occlusionDistanceFactor), 0.0f, 1.0f));
			lightIntensity += sectorLightIntensity;
			direction += sectorLightIntensity * lightDir;
		}
		ratio *= settings.m_layerAngleFactor;
	}
	lightDirection = glm::normalize(direction);
	lightIntensity = glm::clamp(lightIntensity / ratioSum * settings.m_overallIntensity, 0.0f, 1.0f);
	return lightIntensity;
}

float TreeShadowEstimator::IlluminationEstimation(const glm::vec3& position, glm::vec3& lightDirection) const
{
	const auto& data = m_voxel.Peek(position);
	const float lightIntensity = glm::max(0.0f, 1.0f - data.m_shadowIntensity);
	if (lightIntensity == 0.0f)
	{
		lightDirection = glm::vec3(0.0f);
	}
	else
	{
		lightDirection = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) + data.m_shadowDirection);
	}

	return lightIntensity;
}

void TreeShadowEstimator::AddShadowVolume(const ShadowVolume& shadowVolume)
{
	const auto voxelMinBound = m_voxel.GetMinBound();
	const auto dx = m_voxel.GetVoxelDiameter();
	const auto voxelResolution = m_voxel.GetResolution();
	if (m_settings.m_distanceMultiplier == 0.0f) return;
	const float maxRadius = glm::pow(shadowVolume.m_size * m_settings.m_sizeMultiplier / m_settings.m_minShadowIntensity, 1.0f / m_settings.m_distancePowerFactor) / m_settings.m_distanceMultiplier;
	const int xCenter = (shadowVolume.m_position.x - voxelMinBound.x) / dx;
	const int yCenter = (shadowVolume.m_position.y - voxelMinBound.y) / dx;
	const int zCenter = (shadowVolume.m_position.z - voxelMinBound.z) / dx;
	for (int y = glm::clamp(yCenter - static_cast<int>(maxRadius / dx), 0, voxelResolution.y - 1); y <= glm::clamp(yCenter + static_cast<int>(maxRadius / dx), 0, voxelResolution.y - 1); y++)
	{
		for (int x = glm::clamp(xCenter - static_cast<int>(maxRadius / dx), 0, voxelResolution.x - 1); x <= glm::clamp(xCenter + static_cast<int>(maxRadius / dx), 0, voxelResolution.x - 1); x++)
		{
			for (int z = glm::clamp(zCenter - static_cast<int>(maxRadius / dx), 0, voxelResolution.z - 1); z <= glm::clamp(zCenter + static_cast<int>(maxRadius / dx), 0, voxelResolution.z - 1); z++)
			{
				const auto voxelPosition = m_voxel.GetPosition({ x, y, z });
				const auto positionDiff = voxelPosition - shadowVolume.m_position;
				if (positionDiff.y > 0) continue;

				const auto angle = glm::atan(glm::pow(positionDiff.x * positionDiff.x + positionDiff.z * positionDiff.z, 0.5f) / positionDiff.y);
				const auto distance = glm::length(positionDiff);
				const float shadowIntensity = glm::cos(angle) * glm::min(m_settings.m_maxShadowIntensity, shadowVolume.m_size * m_settings.m_sizeMultiplier / glm::pow(glm::max(1.0f, distance * m_settings.m_distanceMultiplier), m_settings.m_distancePowerFactor));
				if (shadowIntensity < m_settings.m_minShadowIntensity) continue;
				const auto direction = glm::normalize(positionDiff);
				auto& data = m_voxel.Ref(glm::ivec3(x, y, z));
				data.m_shadowIntensity += shadowIntensity;
				data.m_shadowDirection += direction * data.m_shadowIntensity;
			}
		}
	}
}

