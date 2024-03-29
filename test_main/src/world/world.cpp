#include "world.hpp"

#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/profiling/profiling.hpp>

#include <higanbana/graphics/common/image_loaders.hpp>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

const char* componentTypeToString(cgltf_type ty) {
  if (ty == cgltf_type_scalar) {
    return "Float";
  } else if (ty == cgltf_type_vec2) {
    return "Float32x2";
  } else if (ty == cgltf_type_vec3) {
    return "Float32x3";
  } else if (ty == cgltf_type_vec4) {
    return "Float32x4";
  } else if (ty == cgltf_type_mat2) {
    return "Mat2";
  } else if (ty == cgltf_type_mat3) {
    return "Mat3";
  } else if (ty == cgltf_type_mat4) {
    return "Mat4";
  } else {
    // Unknown componenty type
    return "cgltf_type_invalid";
  }
}

higanbana::FormatType gltfComponentTypeToFormatType(cgltf_component_type value)
{
  switch (value)
  {
  case cgltf_component_type_r_8:
    return higanbana::FormatType::Int8;
  case cgltf_component_type_r_8u:
    return higanbana::FormatType::Uint8;
  case cgltf_component_type_r_16:
    return higanbana::FormatType::Int16;
  case cgltf_component_type_r_16u:
    return higanbana::FormatType::Uint16;
  case cgltf_component_type_r_32u:
    return higanbana::FormatType::Uint32;
  case cgltf_component_type_r_32f:
    return higanbana::FormatType::Float32;
  case cgltf_component_type_invalid:
    return higanbana::FormatType::Unknown;
  default:
    HIGAN_ASSERT(false, "Unknown gltf component type %d", value);
  }
  return higanbana::FormatType::Unknown;
}

const char* gltfAlphaModeToString(cgltf_alpha_mode value) {
  switch (value)
  {
  case cgltf_alpha_mode_opaque:
    return "cgltf_alpha_mode_opaque";
  case cgltf_alpha_mode_mask:
    return "cgltf_alpha_mode_opaque";
  case cgltf_alpha_mode_blend:
  default:
    return "cgltf_alpha_mode_blend";
  }
}

namespace app
{
MeshData& World::getMesh(int index) {
  return rawMeshData[index];
}
BufferData& World::getBuffer(int index) {
  return rawBufferData[index];
}
TextureData& World::getTexture(int index) {
  return rawTextureData[index];
}

css::Task<void> loadImageAsync(higanbana::FileSystem& fs, TextureData& rtd, std::string filepath) {
  rtd.image = higanbana::textureUtils::loadImageFromFilesystem(fs, filepath, false);
  co_return;
}

css::Task<void> World::loadGLTFSceneCgltfTasked(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string dir) {
  HIGAN_CPU_FUNCTION_SCOPE();
  for (auto&& file : fs.recursiveList(dir, ".gltf"))
  {
    std::string tryL = "trying to load gltf: " + file;
    HIGAN_CPU_BRACKET(tryL.c_str());

    if (!fs.fileExists(file))
      continue;

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    auto fdata = fs.viewToFile(file);

    auto parentDir = fs.directoryPath(file) + "/";
    std::unique_ptr<std::string> parentDirPtr = std::make_unique<std::string>(parentDir);
    
    cgltf_size fsize = fdata.size_bytes();
    cgltf_result result;
    {
      HIGAN_CPU_BRACKET("cgltf_parse");
      result = cgltf_parse(&options, static_cast<const void*>(fdata.data()), fsize, &data);
    }
    if (result == cgltf_result_success)
    {
      auto getName = [](char* name) {
        if (name)
          return std::string(name);
        return std::string("no name");
      };

      /* TODO make awesome stuff */
      std::string gltfFileName = "Load " + file;
      HIGAN_CPU_BRACKET(gltfFileName.c_str());
      HIGAN_LOGi("cgltf: opened %s successfully\n", file.c_str());

      {
        higanbana::unordered_map<cgltf_scene*, higanbana::Id> scenes;
        higanbana::vector<higanbana::Id> sceness;

        auto& childtable = database.get<components::Childs>();
        auto& names = database.get<components::Name>();
        auto& matrices = database.get<components::Matrix>();

        // create entities for all scenenodes
        higanbana::unordered_map<cgltf_node*, higanbana::Id> entityNodes;
        for (cgltf_node& node : higanbana::MemView(data->nodes, data->nodes_count)) {
          auto id = database.createEntity();
          names.insert(id, {getName(node.name)});
          if (node.has_matrix)
          {
            components::Matrix mat{};
            for (int i = 0; i < 16; ++i) {
              mat.mat(i) = node.matrix[i];
            }
            matrices.insert(id, mat);
          }
          database.getTag<components::SceneNode>().insert(id);
          entityNodes[&node] = id;
        }
        // collect all textures
        higanbana::unordered_map<std::string, higanbana::Id> mapToImages;
        {
          HIGAN_CPU_BRACKET("Copy all textures");
          std::shared_ptr<higanbana::vector<int>> ids = std::make_shared<higanbana::vector<int>>();
          ids->resize(data->images_count);
          for (int i = 0; i < data->images_count; ++i) {
            cgltf_image& image = data->images[i];
            auto id = freelistTexture.allocate();
            if (rawTextureData.size() <= id) rawTextureData.resize(id+1);
            (*ids)[i] = id;
            auto ent = database.createEntity();
            auto& table = database.get<components::RawTextureData>();
            table.insert(ent, {id});
            mapToImages[std::string(image.uri)] = ent;
          }
          higanbana::vector<css::Task<void>> tasks;
          for (int i = 0; i < data->images_count; ++i) {
            std::string funnystring = data->images[i].uri;
            if (funnystring[0] == '.')
              funnystring = funnystring.substr(2);
            tasks.emplace_back(loadImageAsync(fs, rawTextureData[(*ids)[i]], parentDir + funnystring));
          }
          for (auto&& task : tasks)
            co_await task;
        }
        higanbana::unordered_map<cgltf_buffer_view*, higanbana::Id> indexToSourceBufferEntity;
        {
          HIGAN_CPU_BRACKET("copy all bufferviews");
          for (auto&& view : higanbana::MemView(data->buffer_views, data->buffer_views_count))
          {
            auto& buf = *view.buffer;
            auto dataView = fs.viewToFile(parentDir+buf.uri);
            std::vector<uint8_t> copy;
            {
              HIGAN_CPU_BRACKET("memcpy!");
              copy.resize(view.size);
              memcpy(copy.data(), dataView.data()+view.offset, view.size);
            }
            HIGAN_ASSERT(!dataView.empty(), "WTF!");
            auto id = freelistBuffer.allocate();
            if (rawBufferData.size() <= id) rawBufferData.resize(id+1);
            //HIGAN_LOGi("buffer: %s %d %zu %zu %zu\n", buf.uri, id, view.offset, view.size, copy.size());
            rawBufferData[id].data = std::move(copy);
            rawBufferData[id].name = buf.uri;

            auto ent = database.createEntity();
            auto& table = database.get<components::RawBufferData>();
            table.insert(ent, {id});
            indexToSourceBufferEntity[&view] = ent;
          }
        }
        // create scene entities and link scenenodes as childs
        {
          HIGAN_CPU_BRACKET("going through scenes");
          for (auto&& scene : higanbana::MemView(data->scenes, data->scenes_count))
          {
            auto id = database.createEntity();
            names.insert(id,{getName(scene.name)});

            higanbana::vector<higanbana::Id> childs;
            for (auto&& sceneChildNode : higanbana::MemView(scene.nodes, scene.nodes_count))
            {
              childs.push_back(entityNodes[sceneChildNode]);
            }
            childtable.insert(id, {childs});
            database.getTag<components::Scene>().insert(id);
            scenes[&scene] = id;
            sceness.push_back(id);
          }
        }
        // find material
        higanbana::unordered_map<cgltf_material*, higanbana::Id> materials;
        {
          HIGAN_CPU_BRACKET("find materials");
          //for (int i = 0; i < model.materials.size(); ++i)
          for (auto&& material : higanbana::MemView(data->materials, data->materials_count))
          {
            //auto& material = model.materials[i];
            MaterialData md{};
            components::MaterialLink rawLink{};
            auto getTextureIndex = [&](cgltf_texture_view index){
              if (index.texture != nullptr)
              {
                return mapToImages[index.texture->image->uri];
              }
              return higanbana::Id(-1);
            };
            //HIGAN_LOGi("material: %s alphaMode : %s\n", material.name ? material.name : "", gltfAlphaModeToString(material.alpha_mode));
            memcpy(md.emissiveFactor.data, material.emissive_factor, sizeof(cgltf_float) * 3);
            md.alphaCutoff = material.alpha_cutoff;
            md.doubleSided = material.double_sided;
            memcpy(md.baseColorFactor.data, material.pbr_metallic_roughness.base_color_factor, sizeof(cgltf_float) * 4); 
            md.metallicFactor = material.pbr_metallic_roughness.metallic_factor;
            md.roughnessFactor = material.pbr_metallic_roughness.roughness_factor;
            // albedo
            rawLink.albedo = getTextureIndex(material.pbr_metallic_roughness.base_color_texture);
            //md.baseColorTexIndex = getTextureIndex(material.pbrMetallicRoughness.baseColorTexture.index);
            // normal
            rawLink.normal = getTextureIndex(material.normal_texture);
            // metallic/roughness
            rawLink.metallicRoughness = getTextureIndex(material.pbr_metallic_roughness.metallic_roughness_texture);
            // occlusion ?? window or something?
            rawLink.occlusion = getTextureIndex(material.occlusion_texture);
            // emissive
            rawLink.emissive = getTextureIndex(material.emissive_texture);

            auto id = database.createEntity();
            materials[&material] = id;
            auto& materials = database.get<MaterialData>();
            materials.insert(id, md);
            auto& materialLink = database.get<components::MaterialLink>();
            materialLink.insert(id, rawLink);
          }
        }

        // create mesh entities
        higanbana::unordered_map<cgltf_mesh*, higanbana::Id> meshes;
        {
          HIGAN_CPU_BRACKET("mesh entity creation");
          for (auto&& mesh : higanbana::MemView(data->meshes, data->meshes_count))
          {
            //HIGAN_LOGi("mesh found: %s with %zu primitives\n", mesh.name, mesh.primitives_count);
            components::Childs childs;

            for (auto&& primitive : higanbana::MemView(mesh.primitives, mesh.primitives_count))
            {
              MeshData md{};

              if (primitive.indices)
              {
                auto& indiceAccessor = *primitive.indices;
                auto& indiceView = *indiceAccessor.buffer_view;
                auto indType = componentTypeToString(indiceAccessor.type);

                auto componentType = gltfComponentTypeToFormatType(indiceAccessor.component_type);
                md.indices.format = higanbana::FormatType::Uint32;
                if (componentType == higanbana::FormatType::Uint16)
                {
                  md.indices.format = higanbana::FormatType::Uint16;
                }

                //HIGAN_LOGi("Indexbuffer: type:%s byteOffset: %zu count:%zu stride:%zu\n", indType, indiceView.offset, indiceAccessor.count, indiceAccessor.stride);

                auto offset = indiceAccessor.offset;
                auto dataSize = indiceAccessor.count * higanbana::formatSizeInfo(md.indices.format).pixelSize;
                md.indices.offset = offset;
                md.indices.size = dataSize;
                md.indices.buffer = indexToSourceBufferEntity[indiceAccessor.buffer_view];
              }

              for (auto&& attribute : higanbana::MemView(primitive.attributes, primitive.attributes_count))
              {
                auto& accessor = *attribute.data;
                auto& bufferView = accessor.buffer_view;
                auto type = componentTypeToString(accessor.type);
                //HIGAN_ASSERT(accessor.byteOffset == 0, "What Accessor has byteoffset?");
                //HIGAN_LOGi("primitiveBufferView: %s type:%s byteOffset: %zu count:%zu stride:%d\n", attribute.name, type, bufferView->offset, accessor.count, accessor.stride);
                //auto& data = model.buffers[bufferView.buffer];
                auto bufferEntity = indexToSourceBufferEntity[accessor.buffer_view];

                auto offset = accessor.offset;
                auto attrName = std::string(attribute.name);
                if (attrName.compare("POSITION") == 0)
                {
                  md.vertices.format = higanbana::FormatType::Float32RGB;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec3, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.vertices.format).pixelSize;
                  //HIGAN_ASSERT(bufferView.byteOffset % higanbana::formatSizeInfo(md.vertices.format).pixelSize == 0, "Wut");
                  md.vertices.size = dataSize;
                  md.vertices.offset = offset;
                  md.vertices.buffer = bufferEntity;
                }
                else if (attrName.compare("NORMAL") == 0)
                {
                  md.normals.format = higanbana::FormatType::Float32RGB;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec3, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.normals.format).pixelSize;
                  md.normals.size = dataSize;
                  md.normals.offset = offset;
                  md.normals.buffer = bufferEntity;
                }
                else if (attrName.compare("TEXCOORD_0") == 0)
                {
                  md.texCoords.format = higanbana::FormatType::Float32RG;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec2, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.texCoords.format).pixelSize;
                  md.texCoords.size = dataSize;
                  md.texCoords.offset = offset;
                  md.texCoords.buffer = bufferEntity;
                }
                else if (attrName.compare("TANGENT") == 0)
                {
                  md.tangents.format = higanbana::FormatType::Float32RGBA;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec4, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.tangents.format).pixelSize;
                  md.tangents.size = dataSize;
                  md.tangents.offset = offset;
                  md.tangents.buffer = bufferEntity;
                }
              }

              auto id = freelistMesh.allocate();
              if (rawMeshData.size() <= id) rawMeshData.resize(id+1);

              rawMeshData[id] = md;

              auto ent = database.createEntity();
              auto& table = database.get<components::RawMeshData>();
              table.insert(ent, {id});
              auto& mattable = database.get<components::MaterialInstance>();
              if (primitive.material)
              {
                mattable.insert(ent, {materials[primitive.material]});
              }
              database.get<components::Name>().insert(ent, components::Name{file});
              // this is actually a single mesh with a single material

              childs.childs.push_back(ent);
            }

            auto ent = database.createEntity();
            if (!childs.childs.empty())
            {
              childtable.insert(ent, std::move(childs));
              database.getTag<components::MeshNode>().insert(ent);
            }
            // this is a gltf mesh, consisting of many Meshes.
            meshes[&mesh] = ent;
          }

          // create camera entities
          higanbana::unordered_map<cgltf_camera*, higanbana::Id> cameras;
          for (auto&& camera : higanbana::MemView(data->cameras, data->cameras_count))
          {
            auto cameraId = database.createEntity();
            names.insert(cameraId, {camera.name});
            database.getTag<components::CameraNode>().insert(cameraId);
            cameras[&camera] = cameraId;
          }

          // link meshes and cameras and other scenenodes to other as childs
          for (auto&& node : higanbana::MemView(data->nodes, data->nodes_count))
          {
            auto id = entityNodes[&node];
            if (node.children_count != 0)
            {
              higanbana::vector<higanbana::Id> childs;
              for (auto&& sceneChildNode : higanbana::MemView(node.children, node.children_count))
              {
                childs.push_back(entityNodes[sceneChildNode]);
              }
              if (!childs.empty())
                childtable.insert(id, {childs});
            }
            if (node.mesh)
            {
              auto& meshtable = database.get<components::Mesh>();
              // we point to Mesh node, which has meshes as it's child.
              // id entity should basically be a "scene", collection of meshes.
              meshtable.insert(id, {meshes[node.mesh]});
            }
            if (node.camera)
            {
              auto& meshtable = database.get<components::Camera>();
              meshtable.insert(id, {cameras[node.camera]});
            }
          }
        }
        // create gltfnode with scenes as childs.
        auto ent = database.createEntity();
        childtable.insert(ent, {sceness});
        names.insert(ent, {file});
        database.getTag<components::GltfNode>().insert(ent);
      }

      cgltf_free(data);
    } else
    {
      HIGAN_LOGi("cgltf: failed to open %s\n", file.c_str());
    }
  }
  co_return;
}

void World::loadGLTFSceneCgltf(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string dir)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  for (auto&& file : fs.recursiveList(dir, ".gltf"))
  {
    std::string tryL = "trying to load gltf: " + file;
    HIGAN_CPU_BRACKET(tryL.c_str());

    if (!fs.fileExists(dir+file))
      continue;

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    auto fdata = fs.viewToFile(dir+file);

    auto parentDir = fs.directoryPath(dir+file) + "/";
    
    cgltf_size fsize = fdata.size_bytes();
    cgltf_result result;
    {
      HIGAN_CPU_BRACKET("cgltf_parse");
      result = cgltf_parse(&options, static_cast<const void*>(fdata.data()), fsize, &data);
    }
    if (result == cgltf_result_success)
    {
      auto getName = [](char* name) {
        if (name)
          return std::string(name);
        return std::string("no name");
      };

      /* TODO make awesome stuff */
      std::string gltfFileName = "Load " + file;
      HIGAN_CPU_BRACKET(gltfFileName.c_str());
      HIGAN_LOGi("cgltf: opened %s successfully\n", file.c_str());

      {
        higanbana::unordered_map<cgltf_scene*, higanbana::Id> scenes;
        higanbana::vector<higanbana::Id> sceness;

        auto& childtable = database.get<components::Childs>();
        auto& names = database.get<components::Name>();
        auto& matrices = database.get<components::Matrix>();

        // create entities for all scenenodes
        higanbana::unordered_map<cgltf_node*, higanbana::Id> entityNodes;
        for (cgltf_node& node : higanbana::MemView(data->nodes, data->nodes_count)) {
          auto id = database.createEntity();
          names.insert(id, {getName(node.name)});
          if (node.has_matrix)
          {
            components::Matrix mat{};
            for (int i = 0; i < 16; ++i) {
              mat.mat(i) = node.matrix[i];
            }
            matrices.insert(id, mat);
          }
          database.getTag<components::SceneNode>().insert(id);
          entityNodes[&node] = id;
        }
        // collect all textures
        higanbana::unordered_map<std::string, higanbana::Id> mapToImages;
        {
          HIGAN_CPU_BRACKET("Copy all textures");
          higanbana::vector<int> ids;
          ids.resize(data->images_count);
          for (int i = 0; i < data->images_count; ++i) {
            cgltf_image& image = data->images[i];
            auto id = freelistTexture.allocate();
            if (rawTextureData.size() <= id) rawTextureData.resize(id+1);
            ids[i] = id;
            auto ent = database.createEntity();
            auto& table = database.get<components::RawTextureData>();
            table.insert(ent, {id});
            mapToImages[std::string(image.uri)] = ent;
          }
          for (int i = 0; i < data->images_count; ++i) {
            cgltf_image& image = data->images[i];
            auto himage = higanbana::textureUtils::loadImageFromFilesystem(fs, parentDir+image.uri, false);
            auto id = ids[i];
            rawTextureData[id].image = himage;
          }
        }
        higanbana::unordered_map<cgltf_buffer_view*, higanbana::Id> indexToSourceBufferEntity;
        {
          HIGAN_CPU_BRACKET("copy all bufferviews");
          for (auto&& view : higanbana::MemView(data->buffer_views, data->buffer_views_count))
          {
            auto& buf = *view.buffer;
            auto dataView = fs.viewToFile(parentDir+buf.uri);
            std::vector<uint8_t> copy;
            {
              HIGAN_CPU_BRACKET("memcpy!");
              copy.resize(view.size);
              memcpy(copy.data(), dataView.data()+view.offset, view.size);
            }
            HIGAN_ASSERT(!dataView.empty(), "WTF!");
            auto id = freelistBuffer.allocate();
            if (rawBufferData.size() <= id) rawBufferData.resize(id+1);
            //HIGAN_LOGi("buffer: %s %d %zu %zu %zu\n", buf.uri, id, view.offset, view.size, copy.size());
            rawBufferData[id].data = std::move(copy);
            rawBufferData[id].name = buf.uri;

            auto ent = database.createEntity();
            auto& table = database.get<components::RawBufferData>();
            table.insert(ent, {id});
            indexToSourceBufferEntity[&view] = ent;
          }
        }
        // create scene entities and link scenenodes as childs
        {
          HIGAN_CPU_BRACKET("going through scenes");
          for (auto&& scene : higanbana::MemView(data->scenes, data->scenes_count))
          {
            auto id = database.createEntity();
            names.insert(id,{getName(scene.name)});

            higanbana::vector<higanbana::Id> childs;
            for (auto&& sceneChildNode : higanbana::MemView(scene.nodes, scene.nodes_count))
            {
              childs.push_back(entityNodes[sceneChildNode]);
            }
            childtable.insert(id, {childs});
            database.getTag<components::Scene>().insert(id);
            scenes[&scene] = id;
            sceness.push_back(id);
          }
        }
        // find material
        higanbana::unordered_map<cgltf_material*, higanbana::Id> materials;
        {
          HIGAN_CPU_BRACKET("find materials");
          //for (int i = 0; i < model.materials.size(); ++i)
          for (auto&& material : higanbana::MemView(data->materials, data->materials_count))
          {
            //auto& material = model.materials[i];
            MaterialData md{};
            components::MaterialLink rawLink{};
            auto getTextureIndex = [&](cgltf_texture_view index){
              if (index.texture != nullptr)
              {
                return mapToImages[index.texture->image->uri];
              }
              return higanbana::Id(-1);
            };
            //HIGAN_LOGi("material: %s alphaMode : %s\n", material.name ? material.name : "", gltfAlphaModeToString(material.alpha_mode));
            memcpy(md.emissiveFactor.data, material.emissive_factor, sizeof(cgltf_float) * 3);
            md.alphaCutoff = material.alpha_cutoff;
            md.doubleSided = material.double_sided;
            memcpy(md.baseColorFactor.data, material.pbr_metallic_roughness.base_color_factor, sizeof(cgltf_float) * 4); 
            md.metallicFactor = material.pbr_metallic_roughness.metallic_factor;
            md.roughnessFactor = material.pbr_metallic_roughness.roughness_factor;
            // albedo
            rawLink.albedo = getTextureIndex(material.pbr_metallic_roughness.base_color_texture);
            //md.baseColorTexIndex = getTextureIndex(material.pbrMetallicRoughness.baseColorTexture.index);
            // normal
            rawLink.normal = getTextureIndex(material.normal_texture);
            // metallic/roughness
            rawLink.metallicRoughness = getTextureIndex(material.pbr_metallic_roughness.metallic_roughness_texture);
            // occlusion ?? window or something?
            rawLink.occlusion = getTextureIndex(material.occlusion_texture);
            // emissive
            rawLink.emissive = getTextureIndex(material.emissive_texture);

            auto id = database.createEntity();
            materials[&material] = id;
            auto& materials = database.get<MaterialData>();
            materials.insert(id, md);
            auto& materialLink = database.get<components::MaterialLink>();
            materialLink.insert(id, rawLink);
          }
        }

        // create mesh entities
        higanbana::unordered_map<cgltf_mesh*, higanbana::Id> meshes;
        {
          HIGAN_CPU_BRACKET("mesh entity creation");
          for (auto&& mesh : higanbana::MemView(data->meshes, data->meshes_count))
          {
            //HIGAN_LOGi("mesh found: %s with %zu primitives\n", mesh.name, mesh.primitives_count);
            components::Childs childs;

            for (auto&& primitive : higanbana::MemView(mesh.primitives, mesh.primitives_count))
            {
              MeshData md{};

              if (primitive.indices)
              {
                auto& indiceAccessor = *primitive.indices;
                auto& indiceView = *indiceAccessor.buffer_view;
                auto indType = componentTypeToString(indiceAccessor.type);

                auto componentType = gltfComponentTypeToFormatType(indiceAccessor.component_type);
                md.indices.format = higanbana::FormatType::Uint32;
                if (componentType == higanbana::FormatType::Uint16)
                {
                  md.indices.format = higanbana::FormatType::Uint16;
                }

                //HIGAN_LOGi("Indexbuffer: type:%s byteOffset: %zu count:%zu stride:%zu\n", indType, indiceView.offset, indiceAccessor.count, indiceAccessor.stride);

                auto offset = indiceAccessor.offset;
                auto dataSize = indiceAccessor.count * higanbana::formatSizeInfo(md.indices.format).pixelSize;
                md.indices.offset = offset;
                md.indices.size = dataSize;
                md.indices.buffer = indexToSourceBufferEntity[indiceAccessor.buffer_view];
              }

              for (auto&& attribute : higanbana::MemView(primitive.attributes, primitive.attributes_count))
              {
                auto& accessor = *attribute.data;
                auto& bufferView = accessor.buffer_view;
                auto type = componentTypeToString(accessor.type);
                //HIGAN_ASSERT(accessor.byteOffset == 0, "What Accessor has byteoffset?");
                //HIGAN_LOGi("primitiveBufferView: %s type:%s byteOffset: %zu count:%zu stride:%d\n", attribute.name, type, bufferView->offset, accessor.count, accessor.stride);
                //auto& data = model.buffers[bufferView.buffer];
                auto bufferEntity = indexToSourceBufferEntity[accessor.buffer_view];

                auto offset = accessor.offset;
                auto attrName = std::string(attribute.name);
                if (attrName.compare("POSITION") == 0)
                {
                  md.vertices.format = higanbana::FormatType::Float32RGB;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec3, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.vertices.format).pixelSize;
                  //HIGAN_ASSERT(bufferView.byteOffset % higanbana::formatSizeInfo(md.vertices.format).pixelSize == 0, "Wut");
                  md.vertices.size = dataSize;
                  md.vertices.offset = offset;
                  md.vertices.buffer = bufferEntity;
                }
                else if (attrName.compare("NORMAL") == 0)
                {
                  md.normals.format = higanbana::FormatType::Float32RGB;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec3, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.normals.format).pixelSize;
                  md.normals.size = dataSize;
                  md.normals.offset = offset;
                  md.normals.buffer = bufferEntity;
                }
                else if (attrName.compare("TEXCOORD_0") == 0)
                {
                  md.texCoords.format = higanbana::FormatType::Float32RG;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec2, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.texCoords.format).pixelSize;
                  md.texCoords.size = dataSize;
                  md.texCoords.offset = offset;
                  md.texCoords.buffer = bufferEntity;
                }
                else if (attrName.compare("TANGENT") == 0)
                {
                  md.tangents.format = higanbana::FormatType::Float32RGBA;
                  HIGAN_ASSERT(accessor.type == cgltf_type_vec4, "Expectations betrayed.");
                  HIGAN_ASSERT(accessor.component_type == cgltf_component_type_r_32f, "Expectations betrayed.");
                  auto dataSize = accessor.count * higanbana::formatSizeInfo(md.tangents.format).pixelSize;
                  md.tangents.size = dataSize;
                  md.tangents.offset = offset;
                  md.tangents.buffer = bufferEntity;
                }
              }

              auto id = freelistMesh.allocate();
              if (rawMeshData.size() <= id) rawMeshData.resize(id+1);

              rawMeshData[id] = md;

              auto ent = database.createEntity();
              auto& table = database.get<components::RawMeshData>();
              table.insert(ent, {id});
              auto& mattable = database.get<components::MaterialInstance>();
              if (primitive.material)
              {
                mattable.insert(ent, {materials[primitive.material]});
              }
              database.get<components::Name>().insert(ent, components::Name{file});
              // this is actually a single mesh with a single material

              childs.childs.push_back(ent);
            }

            auto ent = database.createEntity();
            if (!childs.childs.empty())
            {
              childtable.insert(ent, std::move(childs));
              database.getTag<components::MeshNode>().insert(ent);
            }
            // this is a gltf mesh, consisting of many Meshes.
            meshes[&mesh] = ent;
          }

          // create camera entities
          higanbana::unordered_map<cgltf_camera*, higanbana::Id> cameras;
          for (auto&& camera : higanbana::MemView(data->cameras, data->cameras_count))
          {
            auto cameraId = database.createEntity();
            names.insert(cameraId, {camera.name});
            database.getTag<components::CameraNode>().insert(cameraId);
            cameras[&camera] = cameraId;
          }

          // link meshes and cameras and other scenenodes to other as childs
          for (auto&& node : higanbana::MemView(data->nodes, data->nodes_count))
          {
            auto id = entityNodes[&node];
            if (node.children_count != 0)
            {
              higanbana::vector<higanbana::Id> childs;
              for (auto&& sceneChildNode : higanbana::MemView(node.children, node.children_count))
              {
                childs.push_back(entityNodes[sceneChildNode]);
              }
              if (!childs.empty())
                childtable.insert(id, {childs});
            }
            if (node.mesh)
            {
              auto& meshtable = database.get<components::Mesh>();
              // we point to Mesh node, which has meshes as it's child.
              // id entity should basically be a "scene", collection of meshes.
              meshtable.insert(id, {meshes[node.mesh]});
            }
            if (node.camera)
            {
              auto& meshtable = database.get<components::Camera>();
              meshtable.insert(id, {cameras[node.camera]});
            }
          }
        }
        // create gltfnode with scenes as childs.
        auto ent = database.createEntity();
        childtable.insert(ent, {sceness});
        names.insert(ent, {file});
        database.getTag<components::GltfNode>().insert(ent);
      }

      cgltf_free(data);
    } else
    {
      HIGAN_LOGi("cgltf: failed to open %s\n", file.c_str());
    }
  }
}
void World::loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string dir)
{
  loadGLTFSceneCgltf(database, fs, dir);
}
};