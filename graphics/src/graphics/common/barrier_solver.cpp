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
    void BarrierSolver::addBuffer(int drawCallIndex, int64_t id, ResourceHandle buffer, backend::AccessStage stage, backend::AccessUsage usage)
    {

    }
    void BarrierSolver::addTexture(int drawCallIndex, int64_t id, ResourceHandle texture, ResourceHandle view, int16_t mips, ResourceState access)
    {

    }
    void BarrierSolver::addTexture(int drawCallIndex, int64_t id, ResourceHandle texture, int16_t mips, ResourceState access, SubresourceRange range)
    {

    }
    void BarrierSolver::reset()
    {

    }
  }
}