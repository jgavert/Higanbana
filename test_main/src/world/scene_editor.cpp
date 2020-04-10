#include "scene_editor.hpp"
#include <higanbana/core/entity/query.hpp>
#include "components.hpp"
#include <imgui.h>
#include <optional>

using namespace higanbana;

namespace app
{
void SceneEditor::render(Database<2048>& ecs)
{
  //ImGui::ShowDemoWindow(&m_show);
  
  if (ImGui::Begin("SceneEditor", &m_show)) {
    auto& parents = ecs.get<components::Parent>();
    auto& childsTable = ecs.get<components::Childs>();
    auto& names = ecs.get<components::Name>();
    auto& entityObjects = ecs.getTag<components::EntityObject>();
    auto& visibility = ecs.getTag<components::Visible>();
    auto& selected = ecs.getTag<components::SelectedEntity>();

    higanbana::Id lastSelected;
    bool selectionExists = false;
    queryTag<2048>(pack(selected), [&](higanbana::Id id){
      lastSelected = id;
      selectionExists = true;
    });

    bool spawnEntityInSelected = ImGui::Button("Create"); ImGui::SameLine();
    bool deleteEntitySelected = ImGui::Button("Delete");
    
    if (spawnEntityInSelected) {
      query(pack(childsTable), pack(selected), [&](higanbana::Id id, components::Childs& childs) {
        auto entity = ecs.createEntity();
        visibility.insert(entity);
        names.insert(entity, components::Name{"entity"});
        childsTable.insert(entity, components::Childs{});
        entityObjects.insert(entity);
        parents.insert(entity, components::Parent{id});
        childs.childs.push_back(entity);
      });
    }

    if (deleteEntitySelected) {
      vector<higanbana::Id> entitiesToDelete;
      vector<higanbana::Id> childsToGoThrough;
      query(pack(childsTable), pack(selected, entityObjects), [&](higanbana::Id id, components::Childs& childs) {
        entitiesToDelete.push_back(id);
        childsToGoThrough.insert(childsToGoThrough.end(), childs.childs.begin(), childs.childs.end());
      });
      while (!childsToGoThrough.empty()) {
        auto child = childsToGoThrough.back();
        childsToGoThrough.pop_back();
        entitiesToDelete.push_back(child);
        if (auto childs = childsTable.tryGet(child)) {
          childsToGoThrough.insert(childsToGoThrough.end(), childs->childs.begin(), childs->childs.end());
        }
      }
      if (!entitiesToDelete.empty()) {
        auto first = entitiesToDelete.front();
        auto parent = parents.get(first);
        auto& parentsChilds = childsTable.get(parent.id);
        for (int i = 0; i < parentsChilds.childs.size(); i++) {
          auto id = parentsChilds.childs[i];
          if (id == first) {
            parentsChilds.childs.erase(parentsChilds.childs.begin()+i);
            break;
          }
        }
      }
      for (auto&& ent : entitiesToDelete) {
        ecs.deleteEntity(ent);
      }
    }

    auto forSingleId = [&](higanbana::Id id) -> std::optional<components::Childs>
    {
      std::string nodeName = "";
      if (auto name = names.tryGet(id))
      {
        nodeName += name.value().str;
      }
      nodeName += "##";
      nodeName += std::to_string(id);
      bool isSelected = selected.check(id);
      uint flags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow;
      if (selected.check(id))
        flags |= ImGuiTreeNodeFlags_Selected;
      
      auto opt = childsTable.tryGet(id);
      if (!opt || opt->childs.empty()){
        flags |= ImGuiTreeNodeFlags_Leaf;
      }
      bool isVisible = visibility.check(id);

      ImGui::AlignTextToFramePadding();
      bool treeOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags);
      // is selected
      if (ImGui::IsItemClicked()) {
        selected.toggle(id);
        if (selectionExists && selected.check(id) && id != lastSelected) {
          selected.remove(lastSelected);
        }
      }

      // is visible
      std::string visibilityName = "-.-##";
      if (isVisible)
        visibilityName = "<><>##";
      visibilityName += std::to_string(id);
      ImGui::SameLine();
      if (ImGui::Button(visibilityName.c_str()))
        visibility.toggle(id);
      
      if (treeOpen) {
        if (opt && !opt->childs.empty())
        {
          return opt; // early out to form a tree, defer TreePop to later date.
        }
        ImGui::TreePop();
      }
      return {};
    };
    // go through all scene entity roots and collect childs from them
    queryTag<2048>(pack(ecs.getTag<components::SceneEntityRoot>()), [&](higanbana::Id id)
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
  }

  ImGui::End();
}
};