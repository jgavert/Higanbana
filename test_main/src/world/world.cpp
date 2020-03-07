#include "world.hpp"

#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/profiling/profiling.hpp>
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

higanbana::FormatTypeIdentifier gltfComponentTypeToFormatTypeIdent(int value)
{
  switch (value)
  {
  case TINYGLTF_COMPONENT_TYPE_BYTE:
    return higanbana::FormatTypeIdentifier::Signed;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    return higanbana::FormatTypeIdentifier::Unsigned;
  case TINYGLTF_COMPONENT_TYPE_SHORT:
    return higanbana::FormatTypeIdentifier::Signed;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    return higanbana::FormatTypeIdentifier::Unsigned;
  case TINYGLTF_COMPONENT_TYPE_INT:
    return higanbana::FormatTypeIdentifier::Signed;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    return higanbana::FormatTypeIdentifier::Unsigned;
  case TINYGLTF_COMPONENT_TYPE_FLOAT:
    return higanbana::FormatTypeIdentifier::Float;
  case TINYGLTF_COMPONENT_TYPE_DOUBLE:
    return higanbana::FormatTypeIdentifier::Double;
  default:
    HIGAN_ASSERT(false, "Unknown gltf component type %d", value);
  }
  return higanbana::FormatTypeIdentifier::Unknown;
}

higanbana::FormatType convertTinygltfFormat(int component, int bits, int pixel_type)
{
  auto ident = gltfComponentTypeToFormatTypeIdent(pixel_type);
  if (ident == higanbana::FormatTypeIdentifier::Unsigned)
    ident = higanbana::FormatTypeIdentifier::Unorm;
  auto format = higanbana::findFormat(component, bits, ident);
  HIGAN_ASSERT(format != higanbana::FormatType::Unknown, "Should find a valid format");
  return format;
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
void World::loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string dir)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  for (auto&& file : fs.recursiveList(dir, ".gltf"))
  {
    if (!fs.fileExists(dir+file))
      continue;

    using namespace tinygltf;

    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    gltfUserData usrData = {fs, dir};

    FsCallbacks fscallbacks = {&customFileExists, &customExpandFilePath, &customReadWholeFile, &customWriteWholeFile, &usrData};

    loader.SetFsCallbacks(fscallbacks);

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, dir+file);
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

      // collect all textures
      higanbana::vector<higanbana::Id> indexToRawImages(model.textures.size(), 0);
      for (auto&& tex : model.textures)
      {
        auto imageIdx = tex.source;
        if (imageIdx >= 0)
        {
          auto& image = model.images[imageIdx];
          HIGAN_LOGi("image: %s\n", image.uri.c_str());
          HIGAN_ASSERT(image.bufferView == -1, "WTF!");

          higanbana::ResourceDescriptor desc = higanbana::ResourceDescriptor()
            .setName(image.uri)
            .setSize(uint3(image.width, image.height, 1))
            .setFormat(convertTinygltfFormat(image.component, image.bits, image.pixel_type));
          higanbana::CpuImage himage(desc);
          auto subres = himage.subresource(0,0);
          auto ourSize = subres.size();
          auto theirSize = image.image.size();
          HIGAN_ASSERT(ourSize == theirSize, "sizes should match, about");
          memcpy(subres.data(), image.image.data(), image.image.size());

          auto id = freelistTexture.allocate();
          if (rawTextureData.size() <= id) rawTextureData.resize(id+1);
          rawTextureData[id].image = himage;

          auto ent = database.createEntity();
          auto& table = database.get<components::RawTextureData>();
          table.insert(ent, {id});
          indexToRawImages[imageIdx] = ent;
        }
      }

      // collect all buffers
      higanbana::vector<higanbana::Id> indexToSourceBufferEntity(model.bufferViews.size(), 0);
      int bufferIdx = 0;
      for (auto&& view : model.bufferViews)
      {
        auto& buf = model.buffers[view.buffer];
        auto& data = buf.data;
        std::vector<uint8_t> copy;
        copy.resize(view.byteLength);
        memcpy(copy.data(), data.data()+view.byteOffset, view.byteLength);
        HIGAN_LOGi("buffer: %s\n", buf.uri.c_str());
        HIGAN_ASSERT(!data.empty(), "WTF!");
        auto id = freelistBuffer.allocate();
        if (rawBufferData.size() <= id) rawBufferData.resize(id+1);
        rawBufferData[id].data = std::move(copy);
        rawBufferData[id].name = buf.uri;

        auto ent = database.createEntity();
        auto& table = database.get<components::RawBufferData>();
        table.insert(ent, {id});
        indexToSourceBufferEntity[bufferIdx] = ent;
        bufferIdx++;
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


      // find material
      higanbana::vector<higanbana::Id> materials(model.materials.size(), -1);
      for (int i = 0; i < model.materials.size(); ++i)
      {
        auto& material = model.materials[i];
        MaterialData md{};
        components::MaterialLink rawLink{};
        auto getTextureIndex = [&](int index){
          if (index >= 0)
          {
            auto tex = model.textures[index];
            return indexToRawImages.at(tex.source);
          }
          return higanbana::Id(-1);
        };
        HIGAN_LOGi("material: %s alphaMode : %s\n", material.name.c_str(), material.alphaMode.c_str());
        memcpy(md.emissiveFactor.data, material.emissiveFactor.data(), sizeof(double) * material.emissiveFactor.size());
        md.alphaCutoff = material.alphaCutoff;
        md.doubleSided = material.doubleSided;
        memcpy(md.baseColorFactor.data, material.pbrMetallicRoughness.baseColorFactor.data(), sizeof(double)*material.pbrMetallicRoughness.baseColorFactor.size()); 
        md.metallicFactor = material.pbrMetallicRoughness.metallicFactor;
        md.roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;
        // albedo
        rawLink.albedo = getTextureIndex(material.pbrMetallicRoughness.baseColorTexture.index);
        //md.baseColorTexIndex = getTextureIndex(material.pbrMetallicRoughness.baseColorTexture.index);
        // normal
        rawLink.normal = getTextureIndex(material.normalTexture.index);
        // metallic/roughness
        rawLink.metallicRoughness = getTextureIndex(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
        // occlusion ?? window or something?
        rawLink.occlusion = getTextureIndex(material.occlusionTexture.index);
        // emissive
        rawLink.emissive = getTextureIndex(material.emissiveTexture.index);

        auto id = database.createEntity();
        materials[i] = id;
        auto& materials = database.get<MaterialData>();
        materials.insert(id, md);
        auto& materialLink = database.get<components::MaterialLink>();
        materialLink.insert(id, rawLink);
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
            md.indices.format = higanbana::FormatType::Uint32;
            if (componentType == higanbana::FormatType::Uint16)
            {
              md.indices.format = higanbana::FormatType::Uint16;
            }

            HIGAN_LOGi("Indexbuffer: type:%s byteOffset: %zu count:%zu stride:%d\n", indType, indiceView.byteOffset, indiceAccessor.count, indiceAccessor.ByteStride(indiceView));

            auto offset = indiceAccessor.byteOffset;
            auto dataSize = indiceAccessor.count * higanbana::formatSizeInfo(md.indices.format).pixelSize;
            md.indices.offset = offset;
            md.indices.size = dataSize;
            md.indices.buffer = indexToSourceBufferEntity[indiceAccessor.bufferView];
          }

          for (auto&& attribute : primitive.attributes)
          {
            auto& accessor = model.accessors[attribute.second];
            auto& bufferView = model.bufferViews[accessor.bufferView];
            auto type = componentTypeToString(accessor.type);
            HIGAN_ASSERT(accessor.byteOffset == 0, "What Accessor has byteoffset?");
            HIGAN_LOGi("primitiveBufferView: %s type:%s byteOffset: %zu count:%zu stride:%d\n", attribute.first.c_str(), type, bufferView.byteOffset, accessor.count, accessor.ByteStride(bufferView));
            //auto& data = model.buffers[bufferView.buffer];
            auto bufferEntity = indexToSourceBufferEntity[accessor.bufferView];

            auto offset = accessor.byteOffset;
            if (attribute.first.compare("POSITION") == 0)
            {
              md.vertices.format = higanbana::FormatType::Float32RGB;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC3, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.vertices.format).pixelSize;
              //HIGAN_ASSERT(bufferView.byteOffset % higanbana::formatSizeInfo(md.vertices.format).pixelSize == 0, "Wut");
              md.vertices.size = dataSize;
              md.vertices.offset = offset;
              md.vertices.buffer = bufferEntity;
            }
            else if (attribute.first.compare("NORMAL") == 0)
            {
              md.normals.format = higanbana::FormatType::Float32RGB;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC3, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.normals.format).pixelSize;
              md.normals.size = dataSize;
              md.normals.offset = offset;
              md.normals.buffer = bufferEntity;
            }
            else if (attribute.first.compare("TEXCOORD_0") == 0)
            {
              md.texCoords.format = higanbana::FormatType::Float32RG;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC2, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
              auto dataSize = accessor.count * higanbana::formatSizeInfo(md.texCoords.format).pixelSize;
              md.texCoords.size = dataSize;
              md.texCoords.offset = offset;
              md.texCoords.buffer = bufferEntity;
            }
            else if (attribute.first.compare("TANGENT") == 0)
            {
              md.tangents.format = higanbana::FormatType::Float32RGBA;
              HIGAN_ASSERT(accessor.type == TINYGLTF_TYPE_VEC4, "Expectations betrayed.");
              HIGAN_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expectations betrayed.");
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
          if (primitive.material >= 0)
          {
            mattable.insert(ent, {materials[primitive.material]});
          }

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