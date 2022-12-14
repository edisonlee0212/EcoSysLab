#pragma once

#include "ecosyslab_export.h"
#include "TreeModel.hpp"
#include "Graphics.hpp"
#include "EditorLayer.hpp"
#include "Application.hpp"

using namespace UniEngine;
namespace EcoSysLab {
    enum class PruningMode {
        None,
        Stroke
    };


    class TreeVisualizer {
        std::vector<glm::vec4> m_randomColors;

        std::vector<glm::mat4> m_internodeMatrices;
        std::vector<glm::vec4> m_internodeColors;
        std::vector<glm::mat4> m_rootNodeMatrices;
        std::vector<glm::vec4> m_rootNodeColors;

        std::vector<glm::vec2> m_storedMousePositions;
        bool m_visualization = true;
        bool m_treeHierarchyGui = true;
        bool m_rootHierarchyGui = true;
        NodeHandle m_selectedInternodeHandle = -1;
        float m_selectedInternodeLengthFactor = 0.0f;
        std::vector<NodeHandle> m_selectedInternodeHierarchyList;

        NodeHandle m_selectedRootNodeHandle = -1;
        float m_selectedRootNodeLengthFactor = 0.0f;
        std::vector<NodeHandle> m_selectedRootNodeHierarchyList;

        PruningMode m_mode = PruningMode::None;

        template<typename SkeletonData, typename FlowData, typename NodeData>
        bool
        RayCastSelection(const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                         const GlobalTransform &globalTransform, NodeHandle &selectedNodeHandle,
                         std::vector<NodeHandle> &hierarchyList, float& lengthFactor);

        template<typename SkeletonData, typename FlowData, typename NodeData>
        bool ScreenCurvePruning(Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                                const GlobalTransform &globalTransform, NodeHandle &selectedNodeHandle,
                                std::vector<NodeHandle> &hierarchyList);


        bool DrawInternodeInspectionGui(
                TreeModel &treeModel,
                NodeHandle internodeHandle, bool &deleted,
                const unsigned &hierarchyLevel);

        template<typename SkeletonData, typename FlowData, typename NodeData>
        void PeekNodeInspectionGui(
                const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                NodeHandle nodeHandle, NodeHandle &selectedNodeHandle, std::vector<NodeHandle> &hierarchyList,
                const unsigned &hierarchyLevel);

        bool DrawRootNodeInspectionGui(
                TreeModel &treeModel,
                NodeHandle rootNodeHandle, bool &deleted,
                const unsigned &hierarchyLevel);

        void
        PeekInternode(const Skeleton <SkeletonGrowthData, BranchGrowthData, InternodeGrowthData> &treeSkeleton,
                      NodeHandle internodeHandle);

        bool
        InspectInternode(Skeleton <SkeletonGrowthData, BranchGrowthData, InternodeGrowthData> &treeSkeleton,
                         NodeHandle internodeHandle);

        void
        PeekRootNode(
                const Skeleton <RootSkeletonGrowthData, RootBranchGrowthData, RootInternodeGrowthData> &rootSkeleton,
                NodeHandle rootNodeHandle);

        bool
        InspectRootNode(Skeleton <RootSkeletonGrowthData, RootBranchGrowthData, RootInternodeGrowthData> &rootSkeleton,
                        NodeHandle rootNodeHandle);

    public:
        template<typename SkeletonData, typename FlowData, typename NodeData>
        void SetSelectedNode(
                const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                NodeHandle nodeHandle, NodeHandle &selectedNodeHandle, std::vector<NodeHandle> &hierarchyList);

        template<typename SkeletonData, typename FlowData, typename NodeData>
        void
        SyncMatrices(const Skeleton <SkeletonData, FlowData, NodeData> &skeleton, std::vector<glm::mat4> &matrices,
                     std::vector<glm::vec4> &colors, NodeHandle &selectedNodeHandle, float& lengthFactor);

        int m_iteration = 0;
        bool m_needUpdate = false;

        bool
        OnInspect(TreeModel &treeModel,
                  const GlobalTransform &globalTransform);

        void Reset(TreeModel &treeModel);
    };

    template<typename SkeletonData, typename FlowData, typename NodeData>
    bool TreeVisualizer::ScreenCurvePruning(Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                                            const GlobalTransform &globalTransform, NodeHandle &selectedNodeHandle,
                                            std::vector<NodeHandle> &hierarchyList) {
        auto editorLayer = Application::GetLayer<EditorLayer>();
        const auto cameraRotation = editorLayer->m_sceneCameraRotation;
        const auto cameraPosition = editorLayer->m_sceneCameraPosition;
        const glm::vec3 cameraFront = cameraRotation * glm::vec3(0, 0, -1);
        const glm::vec3 cameraUp = cameraRotation * glm::vec3(0, 1, 0);
        glm::mat4 projectionView = editorLayer->m_sceneCamera->GetProjection() *
                                   glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

        const auto &sortedInternodeList = skeleton.RefSortedNodeList();
        bool changed = false;
        for (const auto &internodeHandle: sortedInternodeList) {
            if (internodeHandle == 0) continue;
            auto &internode = skeleton.RefNode(internodeHandle);
            if (internode.IsRecycled()) continue;
            glm::vec3 position = internode.m_info.m_globalPosition;
            auto rotation = internode.m_info.m_globalRotation;
            const auto direction = glm::normalize(rotation * glm::vec3(0, 0, -1));
            auto position2 =
                    position + internode.m_info.m_length * direction;

            position = (globalTransform.m_value *
                        glm::translate(position))[3];
            position2 = (globalTransform.m_value *
                         glm::translate(position2))[3];
            const glm::vec4 internodeScreenStart4 = projectionView * glm::vec4(position, 1.0f);
            const glm::vec4 internodeScreenEnd4 = projectionView * glm::vec4(position2, 1.0f);
            glm::vec3 internodeScreenStart = internodeScreenStart4 / internodeScreenStart4.w;
            glm::vec3 internodeScreenEnd = internodeScreenEnd4 / internodeScreenEnd4.w;
            internodeScreenStart.x *= -1.0f;
            internodeScreenEnd.x *= -1.0f;
            if (internodeScreenStart.x < -1.0f || internodeScreenStart.x > 1.0f || internodeScreenStart.y < -1.0f ||
                internodeScreenStart.y > 1.0f || internodeScreenStart.z < 0.0f)
                continue;
            if (internodeScreenEnd.x < -1.0f || internodeScreenEnd.x > 1.0f || internodeScreenEnd.y < -1.0f ||
                internodeScreenEnd.y > 1.0f || internodeScreenEnd.z < 0.0f)
                continue;
            bool intersect = false;
            for (int i = 0; i < m_storedMousePositions.size() - 1; i++) {
                auto &lineStart = m_storedMousePositions[i];
                auto &lineEnd = m_storedMousePositions[i + 1];
                float a1 = internodeScreenEnd.y - internodeScreenStart.y;
                float b1 = internodeScreenStart.x - internodeScreenEnd.x;
                float c1 = a1 * (internodeScreenStart.x) + b1 * (internodeScreenStart.y);

                // Line CD represented as a2x + b2y = c2
                float a2 = lineEnd.y - lineStart.y;
                float b2 = lineStart.x - lineEnd.x;
                float c2 = a2 * (lineStart.x) + b2 * (lineStart.y);

                float determinant = a1 * b2 - a2 * b1;
                if (determinant == 0.0f) continue;
                float x = (b2 * c1 - b1 * c2) / determinant;
                float y = (a1 * c2 - a2 * c1) / determinant;
                if (x <= glm::max(internodeScreenStart.x, internodeScreenEnd.x) &&
                    x >= glm::min(internodeScreenStart.x, internodeScreenEnd.x) &&
                    y <= glm::max(internodeScreenStart.y, internodeScreenEnd.y) &&
                    y >= glm::min(internodeScreenStart.y, internodeScreenEnd.y) &&
                    x <= glm::max(lineStart.x, lineEnd.x) &&
                    x >= glm::min(lineStart.x, lineEnd.x) &&
                    y <= glm::max(lineStart.y, lineEnd.y) &&
                    y >= glm::min(lineStart.y, lineEnd.y)) {
                    intersect = true;
                    break;
                }
            }
            if (intersect) {
                skeleton.RecycleNode(internodeHandle);
                changed = true;
            }

        }
        if (changed) {
            selectedNodeHandle = -1;
            hierarchyList.clear();
        }
        return changed;
    }

    template<typename SkeletonData, typename FlowData, typename NodeData>
    bool TreeVisualizer::RayCastSelection(const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                                          const GlobalTransform &globalTransform, NodeHandle &selectedNodeHandle,
                                          std::vector<NodeHandle> &hierarchyList, float& lengthFactor) {
        auto editorLayer = Application::GetLayer<EditorLayer>();
        bool changed = false;

        if (editorLayer->SceneCameraWindowFocused()) {
#pragma region Ray selection
            NodeHandle currentFocusingNodeHandle = -1;
            std::mutex writeMutex;
            float minDistance = FLT_MAX;
            GlobalTransform cameraLtw;
            cameraLtw.m_value =
                    glm::translate(
                            editorLayer->m_sceneCameraPosition) *
                    glm::mat4_cast(
                            editorLayer->m_sceneCameraRotation);
            const Ray cameraRay = editorLayer->m_sceneCamera->ScreenPointToRay(
                    cameraLtw, editorLayer->GetMouseScreenPosition());
            const auto &sortedFlowList = skeleton.RefSortedFlowList();
            const auto &sortedNodeList = skeleton.RefSortedNodeList();
            std::vector<std::shared_future<void>> results;
            Jobs::ParallelFor(sortedNodeList.size(), [&](unsigned i) {
                const auto &node = skeleton.PeekNode(sortedNodeList[i]);
                auto rotation = globalTransform.GetRotation() * node.m_info.m_globalRotation;
                glm::vec3 position = (globalTransform.m_value *
                                      glm::translate(node.m_info.m_globalPosition))[3];
                const auto direction = glm::normalize(rotation * glm::vec3(0, 0, -1));
                const glm::vec3 position2 =
                        position + node.m_info.m_length * direction;
                const auto center =
                        (position + position2) / 2.0f;
                auto radius = node.m_info.m_thickness;
                const auto height = glm::distance(position2,
                                                  position);
                radius *= height / node.m_info.m_length;
                if (!cameraRay.Intersect(center,
                                         height / 2.0f) && !cameraRay.Intersect(center,
                                                                                radius)) {
                    return;
                }
                const auto &dir = -cameraRay.m_direction;
#pragma region Line Line intersection
                /*
    * http://geomalgorithms.com/a07-_distance.html
    */
                glm::vec3 v = position - position2;
                glm::vec3 w = (cameraRay.m_start + dir) - position2;
                const auto a = glm::dot(dir, dir); // always >= 0
                const auto b = glm::dot(dir, v);
                const auto c = glm::dot(v, v); // always >= 0
                const auto d = glm::dot(dir, w);
                const auto e = glm::dot(v, w);
                const auto dotP = a * c - b * b; // always >= 0
                float sc, tc;
                // compute the line parameters of the two closest points
                if (dotP < 0.00001f) { // the lines are almost parallel
                    sc = 0.0f;
                    tc = (b > c ? d / b : e / c); // use the largest denominator
                } else {
                    sc = (b * e - c * d) / dotP;
                    tc = (a * e - b * d) / dotP;
                }
                // get the difference of the two closest points
                glm::vec3 dP = w + sc * dir - tc * v; // =  L1(sc) - L2(tc)
                if (glm::length(dP) > radius)
                    return;
#pragma endregion

                const auto distance = glm::distance(
                        glm::vec3(cameraLtw.m_value[3]),
                        glm::vec3(center));
                std::lock_guard<std::mutex> lock(writeMutex);
                if (distance < minDistance) {
                    minDistance = distance;
                    lengthFactor = glm::clamp(1.0f - tc, 0.0f, 1.0f);
                    currentFocusingNodeHandle = sortedNodeList[i];
                }
            }, results);
            for (auto &i: results) i.wait();
            if (currentFocusingNodeHandle != -1) {
                SetSelectedNode(skeleton, currentFocusingNodeHandle, selectedNodeHandle,
                                hierarchyList);
                changed = true;
            }
#pragma endregion
        }
        return changed;
    }

    template<typename SkeletonData, typename FlowData, typename NodeData>
    void TreeVisualizer::PeekNodeInspectionGui(
            const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
            NodeHandle nodeHandle, NodeHandle &selectedNodeHandle, std::vector<NodeHandle> &hierarchyList,
            const unsigned &hierarchyLevel) {
        const int index = hierarchyList.size() - hierarchyLevel - 1;
        if (!hierarchyList.empty() && index >= 0 &&
            index < hierarchyList.size() &&
            hierarchyList[index] == nodeHandle) {
            ImGui::SetNextItemOpen(true);
        }
        const bool opened = ImGui::TreeNodeEx(("Handle: " + std::to_string(nodeHandle)).c_str(),
                                              ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow |
                                              ImGuiTreeNodeFlags_NoAutoOpenOnLog |
                                              (selectedNodeHandle == nodeHandle ? ImGuiTreeNodeFlags_Framed
                                                                                : ImGuiTreeNodeFlags_FramePadding));
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            SetSelectedNode(skeleton, nodeHandle, selectedNodeHandle, hierarchyList);
        }
        if (opened) {
            ImGui::TreePush();
            const auto &internode = skeleton.PeekNode(nodeHandle);
            const auto &internodeChildren = internode.RefChildHandles();
            for (const auto &child: internodeChildren) {
                PeekNodeInspectionGui(skeleton, child, selectedNodeHandle, hierarchyList, hierarchyLevel + 1);
            }
            ImGui::TreePop();
        }
    }

    template<typename SkeletonData, typename FlowData, typename NodeData>
    void TreeVisualizer::SetSelectedNode(
            const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
            NodeHandle nodeHandle, NodeHandle &selectedNodeHandle, std::vector<NodeHandle> &hierarchyList) {
        if (nodeHandle != selectedNodeHandle) {
            hierarchyList.clear();
            if (nodeHandle < 0) {
                selectedNodeHandle = -1;
            } else {
                selectedNodeHandle = nodeHandle;
                auto walker = nodeHandle;
                while (walker != -1) {
                    hierarchyList.push_back(walker);
                    const auto &internode = skeleton.PeekNode(walker);
                    walker = internode.GetParentHandle();
                }
            }
        }
    }

    template<typename SkeletonData, typename FlowData, typename NodeData>
    void TreeVisualizer::SyncMatrices(const Skeleton <SkeletonData, FlowData, NodeData> &skeleton,
                                      std::vector<glm::mat4> &matrices,
                                      std::vector<glm::vec4> &colors, NodeHandle &selectedNodeHandle, float& lengthFactor) {
        if (m_randomColors.empty()) {
            for (int i = 0; i < 1000; i++) {
                m_randomColors.emplace_back(glm::ballRand(1.0f), 1.0f);
            }
        }
        const auto &sortedNodeList = skeleton.RefSortedNodeList();
        matrices.resize(sortedNodeList.size() + 1);
        colors.resize(sortedNodeList.size() + 1);
        std::vector<std::shared_future<void>> results;
        Jobs::ParallelFor(sortedNodeList.size(), [&](unsigned i) {
            auto nodeHandle = sortedNodeList[i];
            const auto &node = skeleton.PeekNode(nodeHandle);
            glm::vec3 position = node.m_info.m_globalPosition;
            const auto direction = glm::normalize(node.m_info.m_globalRotation * glm::vec3(0, 0, -1));
            auto rotation = glm::quatLookAt(
                    direction, glm::vec3(direction.y, direction.z, direction.x));
            rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
            const glm::mat4 rotationTransform = glm::mat4_cast(rotation);
            matrices[i + 1] =
                    glm::translate(position + (node.m_info.m_length / 2.0f) * direction) *
                    rotationTransform *
                    glm::scale(glm::vec3(
                            node.m_info.m_thickness,
                            node.m_info.m_length / 2.0f,
                            node.m_info.m_thickness));
            if (nodeHandle == selectedNodeHandle) {
                colors[i + 1] = glm::vec4(1, 0, 0, 1);

                const glm::vec3 selectedCenter =
                        position + (node.m_info.m_length * lengthFactor) * direction;
                matrices[0] = glm::translate(selectedCenter) *
                              rotationTransform *
                              glm::scale(glm::vec3(
                                      node.m_info.m_thickness + 0.001f,
                                      node.m_info.m_length / 10.0f,
                                      node.m_info.m_thickness + 0.001f));
                colors[0] = glm::vec4(1.0f);
            } else {
                colors[i + 1] = m_randomColors[skeleton.PeekFlow(node.GetFlowHandle()).m_data.m_order];
                if (selectedNodeHandle != -1) colors[i + 1].a = 0.3f;
            }
        }, results);
        for (auto &i: results) i.wait();
    }
}