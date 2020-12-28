#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/entity/database.hpp>

#include <string>

namespace components
{
struct Position
{
  float3 pos;
};

struct Rotation
{
  quaternion rot;
};

struct Scale
{
  float3 value;
};

struct Matrix
{
  float4x4 mat;
};

struct CameraSettings
{
  float fov;
  float minZ;
  float maxZ;
  float aperture;
  float focusDist;
};

struct Name
{
  std::string str;
};

struct Parent
{
  higanbana::Id id;
};

struct Childs
{
  higanbana::vector<higanbana::Id> childs;
};

struct Mesh
{
  higanbana::Id target;
};

struct Camera
{
  higanbana::Id target;
};

struct RawMeshData
{
  int id;
};
struct RawTextureData
{
  int id;
};
struct RawBufferData
{
  int id;
};

struct MaterialLink
{
  higanbana::Id albedo;
  higanbana::Id normal;
  higanbana::Id metallicRoughness;
  higanbana::Id occlusion;
  higanbana::Id emissive;
};
// links to gpu resources
struct GpuBufferInstance
{
  int id;
};

struct GpuMeshInstance
{
  int id;
};

struct GpuTextureInstance
{
  int id;
};
struct GpuMaterialInstance
{
  int id;
};

// points to a RawMeshData
struct MeshInstance
{
  higanbana::Id id;
};

// instance of a material, edit 1 material to affect many materials
struct MaterialInstance
{
  higanbana::Id id;
};

struct SceneInstance
{
  higanbana::Id target;
};

struct ViewportCamera
{
  int targetViewport;
};

// tags
struct GltfNode; // represents a gltf file as node, these are my current active scenes. has a name
struct CameraNode; // camera node in a gltf scene
struct MeshNode; // gltf mesh consists of many RawMeshData childs
struct Scene; // this entity is base node of a scene
struct SceneNode; // represents a node under a scene
struct ActiveCamera; // viewport renders this camera

// runtime
struct SceneEntityRoot; // scene root
struct EntityObject; // represents a runtime node, can have entities or other EntityObjects as childs
struct Visible; // to hide object from sight... should propagate to all of it's childs...?

// editor
struct SelectedEntity; // tagged the EntityObject which is selected to be shown various information about

enum class EntityComponent {
  Position,
  Rotation,
  Scale,
  MeshInstance,
  MaterialInstance,
  CameraSettings,
  ViewportCamera,
  Count
};
const char* toString(EntityComponent val);
}