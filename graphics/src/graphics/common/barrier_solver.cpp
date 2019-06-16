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
    void BarrierSolver::addBuffer(int drawCallIndex, ViewResourceHandle buffer, ResourceState access)
    {
      //m_bufferStates[buffer].state = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      m_jobs.push_back(DependencyPacket{drawCallIndex, buffer, access});
    }
    void BarrierSolver::addTexture(int drawCallIndex, ViewResourceHandle texture, ResourceState access)
    {
      //m_textureStates[buffer] = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      m_jobs.push_back(DependencyPacket{drawCallIndex, texture, access});
    }
    void BarrierSolver::reset()
    {
      m_jobs.clear();
    }

    void BarrierSolver::makeAllBarriers()
    {
      for (int i = 0; i < m_jobs.size(); ++i)
      {
        F_ILOG("BarrierSolver", "draw index %d %zu", m_jobs[i].drawIndex, m_jobs[i].resource.id);
      }
    }
  }
}