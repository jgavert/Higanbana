#include <graphics/common/barrier_solver.hpp>

namespace faze
{
  namespace backend
  {
    int BarrierSolver::addDrawCall(backend::AccessStage baseFlags)
    {
      auto index = m_drawCallStage.size();
      m_drawCallStage.push_back(baseFlags);
      return index; 
    }
    void BarrierSolver::addBuffer(int drawCallIndex, ResourceHandle buffer, ResourceState access)
    {
      //m_bufferStates[buffer].state = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      m_jobs.push_back(DependencyPacket{drawCallIndex, buffer, access, SubresourceRange{}});
    }
    void BarrierSolver::addTexture(int drawCallIndex, ResourceHandle texture, ResourceHandle view, int16_t mips, ResourceState access)
    {
      SubresourceRange range = {};
      if (view.type == ResourceType::TextureSRV)
      {
        range = m_texSRVran[view];
      }
      else if (view.type == ResourceType::TextureUAV)
      {
        range = m_texUAVran[view];
      }
      else if (view.type == ResourceType::TextureRTV)
      {
        range = m_texRTVran[view];
      }
      else if (view.type == ResourceType::TextureDSV)
      {
        range = m_texDSVran[view];
      }
      //m_textureStates[buffer] = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      m_jobs.push_back(DependencyPacket{drawCallIndex, texture, access, range});
    }
    void BarrierSolver::addTexture(int drawCallIndex, ResourceHandle texture, int16_t mips, ResourceState access, SubresourceRange range)
    {
      m_jobs.push_back(DependencyPacket{drawCallIndex, texture, access, range});
    }
    void BarrierSolver::reset()
    {
      m_jobs.clear();
    }
  }
}