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

  auto hasMesh = [&](higanbana::Id id){
    if (auto meshId = meshes.tryGet(id))
    {
      if (ImGui::TreeNode("mesh"))
      {
        ImGui::Text("%zu", meshId.value().target);
        ImGui::TreePop();
      }
    }
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
        hasMesh(child);
        if (ImGui::TreeNode(sceneName.c_str())) {
          if (auto sceneChilds = childsTable.tryGet(child)) {
            for (auto&& cc : sceneChilds.value().childs) {
              std::string sceneNodeName = "scenenode ";
              if (auto name = names.tryGet(cc))
              {
                sceneNodeName += name.value().str;
              }
              hasMesh(cc);
              if (ImGui::TreeNode(sceneNodeName.c_str())) {
                if (auto snChilds = childsTable.tryGet(cc)) {
                  for (auto&& snc : snChilds.value().childs) {
                    std::string nodeName = "node ";
                    if (auto name = names.tryGet(snc))
                    {
                      nodeName += name.value().str;
                    }
                    if (ImGui::TreeNode(nodeName.c_str())) {
                      hasMesh(snc);
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