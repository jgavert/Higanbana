#include "entity_viewer.hpp"
#include <higanbana/core/entity/query.hpp>
#include "components.hpp"
#include <imgui.h>

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

  query(pack(names, childsTable), pack(ecs.getTag<components::GltfNode>()), [&](higanbana::Id id, components::Name& name, components::Childs& childs)
  {
    //HIGAN_LOGi("found gltfnode: %s\n", name.str.c_str());
    if (ImGui::TreeNode(name.str.c_str()))
    {
      for (auto&& child : childs.childs) {
        std::string sceneName = "scene ";
        if (auto name = names.tryGet(child))
        {
          sceneName += name.value().str;
        }
        sceneName += std::to_string(child);
        if (ImGui::TreeNode(sceneName.c_str())) {
          allChecks(child);
          if (auto sceneChilds = childsTable.tryGet(child)) {
            for (auto&& cc : sceneChilds.value().childs) {
              std::string sceneNodeName = "scenenode ";
              if (auto name = names.tryGet(cc))
              {
                sceneNodeName += name.value().str;
              }
              sceneNodeName += std::to_string(cc);
              if (ImGui::TreeNode(sceneNodeName.c_str())) {
                allChecks(cc);
                if (auto snChilds = childsTable.tryGet(cc)) {
                  for (auto&& snc : snChilds.value().childs) {
                    std::string nodeName = "node ";
                    if (auto name = names.tryGet(snc))
                    {
                      nodeName += name.value().str;
                    }
                    nodeName += std::to_string(snc);
                    if (ImGui::TreeNode(nodeName.c_str())) {
                      allChecks(snc);
                      ImGui::TreePop();
                    }
                  }
                }
                ImGui::TreePop();
              }
            }
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  });
  ImGui::End();
}
};