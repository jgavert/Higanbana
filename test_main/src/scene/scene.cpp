#include "scene.hpp"

#include <higanbana/core/filesystem/filesystem.hpp>
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define TINYGLTF_NO_FS
#define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>

bool customFileExists(const std::string& abs_filename, void* userData)
{
  higanbana::FileSystem& fs = *reinterpret_cast<higanbana::FileSystem*>(userData);
  return fs.fileExists(abs_filename);
}

std::string customExpandFilePath(const std::string& filepath, void* userData)
{
  higanbana::FileSystem& fs = *reinterpret_cast<higanbana::FileSystem*>(userData);
  //HIGAN_ASSERT(false, "WTF %s\n", filepath.c_str());
  std::string newPath = "/";
  newPath += filepath;
  return newPath;
}

bool customReadWholeFile(std::vector<unsigned char>* out, std::string* err, const std::string& filepath, void* userData)
{
  higanbana::FileSystem& fs = *reinterpret_cast<higanbana::FileSystem*>(userData);

  HIGAN_ASSERT(fs.fileExists(filepath), "file didn't exist. %s", filepath.c_str());
  auto mem = fs.readFile(filepath);

  out->resize(mem.size());
  memcpy(out->data(), mem.cdata(), mem.size());
  return true;
}

bool customWriteWholeFile(std::string* err, const std::string& filepath, const std::vector<unsigned char>& contents, void* userData)
{
  higanbana::FileSystem& fs = *reinterpret_cast<higanbana::FileSystem*>(userData);
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

namespace app
{
void Scene::loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path)
{
  if (fs.fileExists(path))
  {
    using namespace tinygltf;

    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    FsCallbacks fscallbacks = {&customFileExists, &customExpandFilePath, &customReadWholeFile, &customWriteWholeFile, &fs};

    loader.SetFsCallbacks(fscallbacks);

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
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
      // find the one model
      for (auto&& mesh : model.meshes)
      {
        HIGAN_LOGi("mesh found: %s with %zu primitives\n", mesh.name.c_str(), mesh.primitives.size());
        for (auto&& primitive : mesh.primitives)
        {
          {
            auto& indiceAccessor = model.accessors[primitive.indices];
            auto& indiceView = model.bufferViews[indiceAccessor.bufferView];
            auto indType = componentTypeToString(indiceAccessor.type);

            HIGAN_LOGi("Indexbuffer: type:%s byteOffset: %zu count:%zu stride:%d\n", indType, indiceAccessor.byteOffset, indiceAccessor.count, indiceAccessor.ByteStride(indiceView));
            auto& data = model.buffers[indiceView.buffer];
            
            uint16_t* dataPtr = reinterpret_cast<uint16_t*>(data.data.data() + indiceView.byteOffset + indiceAccessor.byteOffset);
            for (int i = 0; i < indiceAccessor.count; ++i)
            {
              HIGAN_LOGi("   %u%s", dataPtr[i], ((i+1)%3==0?"\n":""));
            }
          }
          for (auto&& attribute : primitive.attributes)
          {
            auto& accessor = model.accessors[attribute.second];
            auto& bufferView = model.bufferViews[accessor.bufferView];
            auto type = componentTypeToString(accessor.type);
            HIGAN_LOGi("primitiveBufferView: %s type:%s byteOffset: %zu count:%zu stride:%d\n", attribute.first.c_str(), type, accessor.byteOffset, accessor.count, accessor.ByteStride(bufferView));
            auto& data = model.buffers[bufferView.buffer];
            
            float3* dataPtr = reinterpret_cast<float3*>(data.data.data()+accessor.byteOffset);
            for (int i = 0; i < accessor.count; ++i)
            {
              HIGAN_LOGi("   %.2f %.2f %.2f\n", dataPtr[i].x, dataPtr[i].y, dataPtr[i].z);
            }
          }
        }
      }
    }
  }
}
};