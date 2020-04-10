#include "components.hpp"

namespace components
{
const char* toString(EntityComponent val) {
  switch (val) {
    case EntityComponent::Position: return "Position";
    case EntityComponent::Rotation: return "Rotation";
    case EntityComponent::Scale: return "Scale";
    case EntityComponent::MeshInstance: return "Mesh Instance";
    case EntityComponent::MaterialInstance: return "Material Instance";
    case EntityComponent::CameraSettings: return "Camera Settings";
    case EntityComponent::ViewportCamera: return "Viewport Camera";
    default:
      return "Unknown";
  }
}
}