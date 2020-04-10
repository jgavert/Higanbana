#include "entity_editor.hpp"
#include <higanbana/core/entity/query.hpp>
#include "components.hpp"
#include <imgui.h>
#include <optional>

using namespace higanbana;

namespace app
{
void EntityEditor::render(Database<2048>& ecs)
{
  //ImGui::ShowDemoWindow(&m_show);
  if (ImGui::Begin("Entity Editor", &m_show)) {
    auto& parents = ecs.get<components::Parent>();
    auto& childsTable = ecs.get<components::Childs>();
    auto& names = ecs.get<components::Name>();
    auto& entityObjects = ecs.getTag<components::EntityObject>();
    auto& visibility = ecs.getTag<components::Visible>();
    auto& selected = ecs.getTag<components::SelectedEntity>();

    auto componentExists = [&](higanbana::Id entity, components::EntityComponent value) {
      switch (value) {
        case components::EntityComponent::Position:
          return ecs.get<components::Position>().check(entity);
        case components::EntityComponent::Rotation:
          return ecs.get<components::Rotation>().check(entity);
        case components::EntityComponent::Scale:
          return ecs.get<components::Scale>().check(entity);
        case components::EntityComponent::MeshInstance:
          return ecs.get<components::MeshInstance>().check(entity);
        case components::EntityComponent::MaterialInstance:
          return ecs.get<components::MaterialInstance>().check(entity);
        case components::EntityComponent::CameraSettings:
          return ecs.get<components::CameraSettings>().check(entity);
        case components::EntityComponent::ViewportCamera:
          return ecs.get<components::ViewportCamera>().check(entity);
        default:
          return false;
      }
    };

    higanbana::Id selection;
    bool selectionExists = false;
    queryTag<2048>(pack(selected), [&](higanbana::Id id){
      selection = id;
      selectionExists = true;
    });
    std::string nodeName = "No node selected";
    ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_TitleBg);
    if (selectionExists) {
      nodeName = names.get(selection).str;
      color = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);
    }
    ImGui::TextColored(color, nodeName.c_str(), "");
    
    if (!selectionExists) {
      ImGui::End();
      return;
    }

    int chosenComponent = -1;
    if (entityObjects.check(selection) && ImGui::Button("Add Components"))
        ImGui::OpenPopup("ComponentSelection");
    if (ImGui::BeginPopup("ComponentSelection")) {
      ImGui::Text("Components");
      ImGui::Separator();
      for (int i = 0; i < static_cast<int>(components::EntityComponent::Count); i++) {
        auto enumVal = static_cast<components::EntityComponent>(i);
        if (!componentExists(selection, enumVal))
          if (ImGui::Selectable(components::toString(enumVal)))
              chosenComponent = i;
      }
      ImGui::EndPopup();
    }

    // create new component
    if (chosenComponent != -1) {
      auto component = static_cast<components::EntityComponent>(chosenComponent);
      switch (component) {
        case components::EntityComponent::Position: {
          ecs.get<components::Position>().insert(selection, components::Position{});
          break;
        }
        case components::EntityComponent::Rotation: {
          ecs.get<components::Rotation>().insert(selection, components::Rotation{quaternion{0,0,0,1}});
          break;
        }
        case components::EntityComponent::Scale: {
          ecs.get<components::Scale>().insert(selection, components::Scale{float3(1.f, 1.f, 1.f)});
          break;
        }
        case components::EntityComponent::MeshInstance: {
          ecs.get<components::MeshInstance>().insert(selection, components::MeshInstance{});
          break;
        }
        case components::EntityComponent::MaterialInstance: {
          ecs.get<components::MaterialInstance>().insert(selection, components::MaterialInstance{});
          break;
        }
        case components::EntityComponent::CameraSettings: {
          ecs.get<components::CameraSettings>().insert(selection, components::CameraSettings{});
          break;
        }
        case components::EntityComponent::ViewportCamera: {
          ecs.get<components::ViewportCamera>().insert(selection, components::ViewportCamera{});
          break;
        }
        default:
          break;
      }
    }

    // component manipulators
    if (ecs.get<components::Position>().check(selection)) {
      if (ImGui::CollapsingHeader("Position")) {
        auto& val = ecs.get<components::Position>().get(selection);
        ImGui::DragFloat3("##position_value", val.pos.data);
      }
    }
    if (ecs.get<components::Rotation>().check(selection)) {
      if (ImGui::CollapsingHeader("Rotation")) {
        auto& rot = ecs.get<components::Rotation>().get(selection);
        float3 dir = math::normalize(rotateVector({ 0.f, 0.f, -1.f }, rot.rot));
        float3 updir = math::normalize(rotateVector({ 0.f, 1.f, 0.f }, rot.rot));
        float3 sideVec = math::normalize(rotateVector({ 1.f, 0.f, 0.f }, rot.rot));
        float3 xyz{};
        ImGui::DragFloat3("xyz##Rotation", xyz.data, 0.01f);
        ImGui::DragFloat4("quaternion##Rotation", rot.rot.data, 0.01f);
        ImGui::DragFloat3("forward##Rotation", dir.data);
        ImGui::DragFloat3("side##Rotation", sideVec.data);
        ImGui::DragFloat3("up##Rotation", updir.data);
        quaternion yaw = math::rotateAxis(updir, xyz.x);
        quaternion pitch = math::rotateAxis(sideVec, xyz.y);
        quaternion roll = math::rotateAxis(dir, xyz.z);
        rot.rot = math::mul(math::mul(math::mul(yaw, pitch), roll), rot.rot);
      }
    }
    if (ecs.get<components::Scale>().check(selection)) {
      if (ImGui::CollapsingHeader("Scale")) {
        auto& val = ecs.get<components::Scale>().get(selection);
        auto multiplier = math::length(val.value);
        ImGui::DragFloat("##scale_value_multi", &multiplier);
        ImGui::DragFloat3("##scale_value", val.value.data);
        val.value = mul(normalize(val.value), multiplier);
      }
    }
    if (ecs.get<components::MeshInstance>().check(selection)) {
      if (ImGui::CollapsingHeader("Mesh Instance")) {
        auto& value = ecs.get<components::MeshInstance>().get(selection);
        auto meshNameC = ecs.get<components::Name>().tryGet(value.id);
        std::string name = (meshNameC && !meshNameC->str.empty()) ? meshNameC->str : std::to_string(value.id);
        name += "##" + std::to_string(value.id);
        ImGui::Text("Mesh: "); ImGui::SameLine();
        if (entityObjects.check(selection) && ImGui::Button(name.c_str()))
          ImGui::OpenPopup("MeshInstanceSelection##entity_editor");
        if (ImGui::BeginPopup("MeshInstanceSelection##entity_editor")) {
          ImGui::Text("Meshes");
          ImGui::Separator();
          query(pack(ecs.get<components::RawMeshData>(), ecs.get<components::Name>()), [&](higanbana::Id id, components::RawMeshData, const components::Name& meshName){
            std::string mname = meshName.str;
            mname += "/" + std::to_string(id);
            mname += "##" + std::to_string(id);
            if (ImGui::Selectable(mname.c_str())){
              value.id = id;
              if (ecs.get<components::MaterialInstance>().check(id)) {
                ecs.get<components::MaterialInstance>().insert(selection, ecs.get<components::MaterialInstance>().get(id));
              }
            }
          });
          ImGui::EndPopup();
        }
      }
    }
    if (ecs.get<components::MaterialInstance>().check(selection)) {
      if (ImGui::CollapsingHeader("Material Instance")) {
        auto& value = ecs.get<components::MaterialInstance>().get(selection);
        auto matNameC = ecs.get<components::Name>().tryGet(value.id);
        std::string name = (matNameC && !matNameC->str.empty()) ? matNameC->str : std::to_string(value.id);
        name += "##" + std::to_string(value.id);
        ImGui::Text("Material: "); ImGui::SameLine();
        if (entityObjects.check(selection) && ImGui::Button(name.c_str()))
          ImGui::OpenPopup("MaterialInstanceSelection##entity_editor");
        if (ImGui::BeginPopup("MaterialInstanceSelection##entity_editor")) {
          ImGui::Text("Materials");
          ImGui::Separator();
          query(pack(ecs.get<components::MaterialLink>()), [&](higanbana::Id id, components::MaterialLink){
            std::string mname = "";
            mname = "unnamed material " + std::to_string(id);
            mname += "##" + std::to_string(id);
            if (ImGui::Selectable(mname.c_str())){
              value.id = id;
            }
          });
          ImGui::EndPopup();
        }
      }
    }
    if (ecs.get<components::CameraSettings>().check(selection)) {
      if (ImGui::CollapsingHeader("Camera Settings")) {
        auto& set = ecs.get<components::CameraSettings>().get(selection);
        ImGui::DragFloat("fov##camera_settings", &set.fov, 0.1f);
        ImGui::DragFloat("minZ##camera_settings", &set.minZ, 0.1f);
        ImGui::DragFloat("maxZ##camera_settings", &set.maxZ, 1.f);
      }
    }
    if (ecs.get<components::ViewportCamera>().check(selection)) {
      if (ImGui::CollapsingHeader("Viewport Camera")) {
        auto& value = ecs.get<components::ViewportCamera>().get(selection);
        ImGui::DragInt("##target_viewport", &value.targetViewport);
      }
    }
  }

  ImGui::End();
}
};