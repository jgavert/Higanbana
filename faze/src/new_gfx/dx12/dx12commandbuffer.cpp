#include "dx12resources.hpp"
#include "util/dxDependencySolver.hpp"

namespace faze
{
  namespace backend
  {
    void handle(ID3D12GraphicsCommandList* buffer, gfxpacket::ClearRT& packet)
    {
      auto view = std::static_pointer_cast<DX12TextureView>(packet.rtv.native());
      auto texture = std::static_pointer_cast<DX12Texture>(packet.rtv.texture().native());
      float rgba[4]{ packet.color.x(), packet.color.y(), packet.color.z(), packet.color.w() };
      buffer->ClearRenderTargetView(view->native().cpu, rgba, 0, nullptr);
    }

    void addCommands(ID3D12GraphicsCommandList* buffer, DX12DependencySolver& solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;
      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
        case CommandPacket::PacketType::ClearRT:
        {
          solver.runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::ClearRT, packet));
          drawIndex++;
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          solver.runBarrier(buffer, drawIndex);
          drawIndex++;
        }
        default:
          break;
        }
      }
    }

    void addDepedencyDataAndSolve(DX12DependencySolver& solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;

      auto addTextureView = [&](int index, TextureView& texView, D3D12_RESOURCE_STATES flags)
      {
        auto view = std::static_pointer_cast<DX12TextureView>(texView.native());
        auto texture = std::static_pointer_cast<DX12Texture>(texView.texture().native());
        solver.addTexture(index, texView.texture().id(), *texture, *view, static_cast<int16_t>(texView.texture().desc().desc.miplevels), flags);
      };

      auto addTexture = [&](int index, Texture& texture, D3D12_RESOURCE_STATES flags)
      {
        auto tex = std::static_pointer_cast<DX12Texture>(texture.native());
        SubresourceRange range{};
        range.mipOffset = 0;
        range.mipLevels = 1;
        range.sliceOffset = 0;
        range.arraySize = 1;
        solver.addTexture(index, texture.id(), *tex, static_cast<int16_t>(texture.desc().desc.miplevels), flags, range);
      };

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
          //        case CommandPacket::PacketType::BufferCopy:
          //        case CommandPacket::PacketType::Dispatch:
        case CommandPacket::PacketType::ClearRT:
        {
          auto& p = packetRef(gfxpacket::ClearRT, packet);
          drawIndex = solver.addDrawCall(packet->type());
          addTextureView(drawIndex, p.rtv, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          auto& p = packetRef(gfxpacket::PrepareForPresent, packet);
          drawIndex = solver.addDrawCall(packet->type());
          addTexture(drawIndex, p.texture, D3D12_RESOURCE_STATE_PRESENT);
          drawIndex++;
        }
        default:
          break;
        }
      }
      solver.makeAllBarriers();
    }

    // implementations
    void DX12CommandBuffer::fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::IntermediateList& list)
    {
      DX12DependencySolver solver;

      addDepedencyDataAndSolve(solver, list);
      addCommands(commandList.Get(), solver, list);

      commandList->Close();
      closedList = true;
    }
  }
}