#include "scene/scene.hpp"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define TINYGLTF_NO_FS
#define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>

bool customFileExists(const std::string& abs_filename, void* userData)
{
  FileSystem& fs = *reinterpret_cast<FileSystem*>(userData);
  return fs.fileExists(abs_filename);
}

std::string customExpandFilePath(const std::string& filepath, void* userData)
{
  FileSystem& fs = *reinterpret_cast<FileSystem*>(userData);
  //HIGAN_ASSERT(false, "WTF %s\n", filepath.c_str());
  std::string newPath = "/";
  newPath += filepath;
  return newPath;
}

bool customReadWholeFile(std::vector<unsigned char>* out, std::string* err, const std::string& filepath, void* userData)
{
  FileSystem& fs = *reinterpret_cast<FileSystem*>(userData);

  HIGAN_ASSERT(fs.fileExists(filepath), "file didn't exist. %s", filepath.c_str());
  auto mem = fs.readFile(filepath);

  out->resize(mem.size());
  memcpy(out->data(), mem.cdata(), mem.size());
  return true;
}

bool customWriteWholeFile(std::string* err, const std::string& filepath, const std::vector<unsigned char>& contents, void* userData)
{
  FileSystem& fs = *reinterpret_cast<FileSystem*>(userData);
  fs.writeFile(filepath, contents);
  return true;
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
  }
}
};