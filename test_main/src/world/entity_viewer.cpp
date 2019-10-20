#include "entity_viewer.hpp"
#include <higanbana/core/entity/query.hpp>
#include "components.hpp"
#include <imgui.h>
#include <optional>

using namespace higanbana;

namespace app
{
void EntityView::render(Database<2048>& ecs)
{
  //ImGui::ShowDemoWindow(&m_show);
  /*
  */
  ImGui::Begin("EntityViewer", &m_show, ImGuiWindowFlags_AlwaysAutoResize);
  auto& childsTable = ecs.get<components::Childs>();
  auto& names = ecs.get<components::Name>();
  auto& meshes = ecs.get<components::Mesh>();
  auto& matrices = ecs.get<components::Matrix>();

  auto& activeScenes = ecs.get<components::SceneInstance>();
  higanbana::Id oldActiveScene = 0;
  bool wasActive = false;

  higanbana::Id activeGameObject;

  query(pack(activeScenes), [&](higanbana::Id id, components::SceneInstance target){
    oldActiveScene = target.target;
    activeGameObject = id;
    wasActive = true;
  });

  if (!wasActive)
  {
    auto id = ecs.createEntity();
    activeScenes.insert(id, components::SceneInstance{oldActiveScene});
    ecs.get<components::WorldPosition>().insert(id, components::WorldPosition{float3(0,0,0)});
    activeGameObject = id;
  }

  // choose active gltf scene... take first one for now.
  auto nameInstance = names.tryGet(oldActiveScene);
  std::string currentSceneStr = "null";
  if (nameInstance)
  {
    currentSceneStr = nameInstance.value().str;
  }
  bool activeSceneSet = false;
  higanbana::Id newActiveScene = oldActiveScene;
  if (ImGui::BeginCombo("Active GLTF scene", currentSceneStr.c_str()))
  {
    query(pack(names), pack(ecs.getTag<components::GltfNode>()), [&](higanbana::Id id, components::Name& name)
    {
      bool selected = id == newActiveScene;
      if (ImGui::Selectable(name.str.c_str(), selected))
      {
        newActiveScene = id;
      }
      if (selected)
      {
        ImGui::SetItemDefaultFocus();
      }
    });
    ImGui::EndCombo();
  }

  activeScenes.insert(activeGameObject, components::SceneInstance{newActiveScene});


  // viewer part functionality
  ImGui::NewLine();
  auto hasMesh = [&](higanbana::Id id){
    if (auto meshId = meshes.tryGet(id))
    {
      std::string meshn = "mesh " + std::to_string(meshId.value().target);
      ImGui::Text(meshn.c_str());
    }
  };

  auto hasMatrix = [&](higanbana::Id id){
    if (auto matrix = matrices.tryGet(id))
    {
      std::string matn = "matrics " + std::to_string(id);
      if (ImGui::TreeNode(matn.c_str()))
      {
        float4x4 val = matrix.value().mat;
        matn.clear();
        for (int i = 0; i < 16;++i)
        {
          matn += " " + std::to_string(val(i));
          if ((i+1) % 4 == 0)
          {
            matn += "\n";
            //ImGui::Text(matn.c_str());
            //ImGui::NewLine();
            //matn.clear();
          }
        }
        ImGui::Text(matn.c_str());
        ImGui::TreePop();
      }
    }
  };

  auto allChecks = [&](higanbana::Id id)
  {
    hasMesh(id);
    hasMatrix(id);
  };

  auto forSingleId = [&](higanbana::Id id) -> std::optional<components::Childs>
  {
    std::string nodeName = "node ";
    if (auto name = names.tryGet(id))
    {
      nodeName += name.value().str;
    }
    nodeName += std::to_string(id);
    if (ImGui::TreeNode(nodeName.c_str())) {
      allChecks(id);
      if (auto opt = childsTable.tryGet(id))
      {
        return opt; // early out to form a tree, defer TreePop to later date.
      }
      ImGui::TreePop();
    }
    return {};
  };

  query(pack(names, childsTable), pack(ecs.getTag<components::GltfNode>()), [&](higanbana::Id id, components::Name& name, components::Childs& childs)
  {
    higanbana::deque<higanbana::vector<higanbana::Id>> stack;
    auto clds = forSingleId(id);
    if (clds)
    {
      // memoize tree traversal
      stack.push_back(clds.value().childs);
      while (!stack.empty())
      {
        if (stack.back().empty())
        {
          stack.pop_back();
          ImGui::TreePop(); // to match ImGui TreeNode counts.
          continue;
        }
        auto& workingSet = stack.back();
        auto val = workingSet.back();
        workingSet.pop_back();
        if (auto new_chlds = forSingleId(val))
        {
          stack.push_back(new_chlds.value().childs);
        }
      }
    }
  });

  ImGui::End();
}
};