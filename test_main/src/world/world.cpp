#include "world.hpp"

#include <higanbana/core/filesystem/filesystem.hpp>
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define TINYGLTF_NO_FS
#define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>

struct gltfUserData
{
  higanbana::FileSystem& fs;
  std::string prefixFolder;
};

bool customFileExists(const std::string& abs_filename, void* userData)
{
  higanbana::FileSystem& fs = reinterpret_cast<gltfUserData*>(userData)->fs;
  return fs.fileExists(abs_filename);
}

std::string customExpandFilePath(const std::string& filepath, void* userData)
{
  higanbana::FileSystem& fs = reinterpret_cast<gltfUserData*>(userData)->fs;
  //HIGAN_ASSERT(false, "WTF %s\n", filepath.c_str());
  //std::string newPath = reinterpret_cast<gltfUserData*>(userData)->prefixFolder + "/";
  //newPath += filepath;
  return filepath;
}

bool customReadWholeFile(std::vector<unsigned char>* out, std::string* err, const std::string& filepath, void* userData)
{
  higanbana::FileSystem& fs = reinterpret_cast<gltfUserData*>(userData)->fs;

  HIGAN_ASSERT(fs.fileExists(filepath), "file didn't exist. %s", filepath.c_str());
  auto mem = fs.readFile(filepath);

  out->resize(mem.size());
  memcpy(out->data(), mem.cdata(), mem.size());
  return true;
}

bool customWriteWholeFile(std::string* err, const std::string& filepath, const std::vector<unsigned char>& contents, void* userData)
{
  higanbana::FileSystem& fs = reinterpret_cast<gltfUserData*>(userData)->fs;
  fs.writeFile(filepath, contents);
  return true;
}

const char* componentTypeToString(uint32_t ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return "Float";
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return "Float32x2";
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return "Float32x3";
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return "Float32x4";
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return "Mat2";
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return "Mat3";
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return "Mat4";
  } else {
    // Unknown componenty type
    return "Unknown";
  }
}

higanbana::FormatType gltfComponentTypeToFormatType(int value)
{
  switch (value)
  {
  case TINYGLTF_COMPONENT_TYPE_BYTE:
    return higanbana::FormatType::Int8;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    return higanbana::FormatType::Uint8;
  case TINYGLTF_COMPONENT_TYPE_SHORT:
    return higanbana::FormatType::Int16;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    return higanbana::FormatType::Uint16;
  case TINYGLTF_COMPONENT_TYPE_INT:
    return higanbana::FormatType::Int32;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    return higanbana::FormatType::Uint32;
  case TINYGLTF_COMPONENT_TYPE_FLOAT:
    return higanbana::FormatType::Float32;
  case TINYGLTF_COMPONENT_TYPE_DOUBLE:
    return higanbana::FormatType::Double;
  default:
    HIGAN_ASSERT(false, "Unknown gltf component type %d", value);
  }
  return higanbana::FormatType::Unknown;
}

namespace app
{
  MeshData& World::getMesh(int index) {
    return rawMeshData[index];
  }
void World::loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string dir)
{
  for (auto&& file : fs.recursiveList(dir, ".gltf"))
  {
    if (!fs.fileExists(dir+"/"+file))
      continue;

    using namespace tinygltf;

    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    gltfUserData usrData = {fs, dir};

    FsCallbacks fscallbacks = {&customFileExists, &customExpandFilePath, &customReadWholeFile, &customWriteWholeFile, &usrData};

    loader.SetFsCallbacks(fscallbacks);

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, dir+"/"+file);
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

    if (!warn.empty()) {
      HIGAN_LOGi("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
      HIGAN_LOGi("Err: %s\n", err.c_str());
    }

    if (!ret) {
      HIGAN_LOGi("Failed to parse glTF\n");
    }

    if (ret)
    {
      higanbana::vector<higanbana::Id> scenes;

      auto& childtable = database.get<components::Childs>();
      auto& names = database.get<components::Name>();
      auto& matrices = database.get<components::Matrix>();

      // create entities for all scenenodes
      higanbana::vector<higanbana::Id> entityNodes;
      for (auto&& node : model.nodes)
      {
        auto id = database.createEntity();
        names.insert(id, {node.name});
        if (!node.matrix.empty())
        {
          components::Matrix mat{};
          for (int i = 0 ; i < node.matrix.size(); ++i)
          {
            mat.mat(i) = node.matrix[i];
          }
          matrices.insert(id, mat);
        }
        database.getTag<components::SceneNode>().insert(id);
        entityNodes.emplace_back(id);
      }

      // create scene entities and link scenenodes as childs
      for (auto&& scene : model.scenes)
      {
        auto id = database.createEntity();
        names.insert(id,{scene.name});

        higanbana::vector<higanbana::Id> childs;
        for (auto&& sceneChildNode : scene.nodes)
        {
          childs.push_back(entityNodes[sceneChildNode]);
        }
        childtable.insert(id, {childs});
        database.getTag<components::Scene>().insert(id);
        scenes.push_back(id);
      }

      // create mesh entities
      higanbana::vector<higanbana::Id> meshes;
      for (auto&& mesh : model.meshes)
      {
        HIGAN_LOGi("mesh found: %s with %zu primitives\n", mesh.name.c_str(), mesh.primitives.size());
        components::Childs childs;
        for (auto&& primitive : mesh.primitives)
        {
          MeshData md{};

          if (primitive.indices >= 0)
          {
            auto& indiceAccessor = model.accessors[primitive.indices];
            auto& indiceView = model.bufferViews[indiceAccessor.bufferView];
            auto indType = componentTypeToString(indiceAccessor.type);

            auto componentType = gltfComponentTypeToFormatType(indiceAccessor.componentType);
            md.indiceFormat = higanbana::FormatType::Uint8;
            if (componentType == higanbana::FormatType::Uint16)
            {
              md.indiceFormat = higanbana::FormatType::Uint16;
            }

            HIGAN_LOGi("Indexbuffer: type:%s byteOffset: %zu count:%zu stride:%d\n", indType, indiceAccessor.byteOffset, indiceAccessor.count, indiceAccessor.ByteStride(indiceView));
            auto& data = model.buffers[indiceView.buffer];

            auto offset = indiceView.byteOffset + indiceAccessor.byteOffset;
            auto dataSize = indiceAccessor.count * higanbana::formatSizeInfo(md.indiceFormat).pixelSize;
            md.indices.resize(dataSize);
            memcpy(md.indices.data(), data.data.data()+offset, dataSize); 
          }


          for (auto&& attribute : primitive.attributes)
          {
            auto& accessor = model.accessors[attribute.second];
            auto& bufferView = model.bufferViews[accessor.bufferView];
            auto type = componentTypeToString(accessor.type);
            HIGAN_LOGi("primitiveBufferView: %s type:%s byteOffset: %zu count:%zu stride:%d\n", attribute.first.c_str(), type, accessor.byteOffset, accessor.count, accessor.ByteStride(bufferView));
            auto& data = model.buffers[bufferView.buffer];

            auto offset = bufferView.byteOffset + accessor.byteOffset;
            if (attribute.first.compare("POSITION") == 0)
            {
              md.vertexFormat = higanbana::FormatType::Float32RGB;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC3, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.vertexFormat).pixelSize;
              md.vertices.resize(dataSize);
              memcpy(md.vertices.data(), data.data.data()+offset, dataSize); 
            }
            else if (attribute.first.compare("NORMAL") == 0)
            {
              md.normalFormat = higanbana::FormatType::Float32RGB;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC3, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.normalFormat).pixelSize;
              md.normals.resize(dataSize);
              memcpy(md.normals.data(), data.data.data()+offset, dataSize); 
            }
            else if (attribute.first.compare("TEXCOORD_0") == 0)
            {
              md.texCoordFormat = higanbana::FormatType::Float32RG;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC2, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.texCoordFormat).pixelSize;
              md.texCoords.resize(dataSize);
              memcpy(md.texCoords.data(), data.data.data()+offset, dataSize); 
            }
            else if (attribute.first.compare("TANGENT") == 0)
            {
              md.tangentFormat = higanbana::FormatType::Float32RGBA;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC4, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.tangentFormat).pixelSize;
              md.tangents.resize(dataSize);
              memcpy(md.tangents.data(), data.data.data()+offset, dataSize); 
            }
          }

          auto id = freelist.allocate();
          if (rawMeshData.size() <= id) rawMeshData.resize(id+1);

          rawMeshData[id] = md;

          auto ent = database.createEntity();
          auto& table = database.get<components::RawMeshData>();
          table.insert(ent, {id});

          childs.childs.push_back(ent);
        }

        auto ent = database.createEntity();
        if (!childs.childs.empty())
        {
          childtable.insert(ent, std::move(childs));
          database.getTag<components::MeshNode>().insert(ent);
        }
        meshes.push_back(ent);
      }

      // create camera entities
      higanbana::vector<higanbana::Id> cameras;
      for (auto&& camera : model.cameras)
      {
        auto cameraId = database.createEntity();
        names.insert(cameraId, {camera.name});
        database.getTag<components::CameraNode>().insert(cameraId);
        cameras.push_back(cameraId);
      }

      // link meshes and cameras and other scenenodes to other as childs
      for (int i = 0; i < model.nodes.size(); ++i)
      {
        auto id = entityNodes[i];
        if (!model.nodes[i].children.empty())
        {
          higanbana::vector<higanbana::Id> childs;
          for (auto&& sceneChildNode : model.nodes[i].children)
          {
            childs.push_back(entityNodes[sceneChildNode]);
          }
          if (!childs.empty())
            childtable.insert(id, {childs});
        }
        if (model.nodes[i].mesh >= 0)
        {
          auto& meshtable = database.get<components::Mesh>();
          meshtable.insert(id, {meshes[model.nodes[i].mesh]});
        }
        if (model.nodes[i].camera >= 0)
        {
          auto& meshtable = database.get<components::Camera>();
          meshtable.insert(id, {cameras[model.nodes[i].camera]});
        }
      }

      // create gltfnode with scenes as childs.
      auto ent = database.createEntity();
      childtable.insert(ent, {scenes});
      names.insert(ent, {file});
      database.getTag<components::GltfNode>().insert(ent);
    }
  }
}
};