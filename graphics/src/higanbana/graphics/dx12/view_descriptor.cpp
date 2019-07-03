#include "higanbana/graphics/dx12/view_descriptor.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/util/formats.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
  namespace backend
  {
    namespace dx12
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC getSRV(ResourceDescriptor& resDesc, ShaderViewDescriptor& view)
      {
        auto& desc = resDesc.desc;

        FormatType format = view.m_format;
        if (format == FormatType::Unknown)
          format = desc.format;

        bool isArray = desc.arraySize > 1;
        int mipLevels = view.m_mipLevels;
        int arraySize = view.m_arraySize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
        srvdesc.Format = formatTodxFormat(format).view;

        if (desc.dimension == FormatDimension::Texture1D)
        {
          if (!isArray)
          {
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvdesc.Texture1D.MipLevels = mipLevels;
            srvdesc.Texture1D.MostDetailedMip = view.m_mostDetailedMip;
            srvdesc.Texture1D.ResourceMinLODClamp = view.m_resourceMinLODClamp;
          }
          else
          {
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvdesc.Texture1DArray.ArraySize = arraySize;
            srvdesc.Texture1DArray.FirstArraySlice = view.m_arraySlice;
            srvdesc.Texture1DArray.MipLevels = mipLevels;
            srvdesc.Texture1DArray.MostDetailedMip = view.m_mostDetailedMip;
            srvdesc.Texture1DArray.ResourceMinLODClamp = view.m_resourceMinLODClamp;
          }
        }
        else if (desc.dimension == FormatDimension::Texture2D)
        {
          if (desc.msCount == 1)
          {
            if (!isArray)
            {
              srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
              srvdesc.Texture2D.PlaneSlice = view.m_planeSlice;
              srvdesc.Texture2D.MipLevels = mipLevels;
              srvdesc.Texture2D.MostDetailedMip = view.m_mostDetailedMip;
              srvdesc.Texture2D.ResourceMinLODClamp = view.m_resourceMinLODClamp;
            }
            else
            {
              srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
              srvdesc.Texture2DArray.PlaneSlice = view.m_planeSlice;
              srvdesc.Texture2DArray.ArraySize = arraySize;
              srvdesc.Texture2DArray.FirstArraySlice = view.m_arraySlice;
              srvdesc.Texture2DArray.MipLevels = mipLevels;
              srvdesc.Texture2DArray.MostDetailedMip = view.m_mostDetailedMip;
              srvdesc.Texture2DArray.ResourceMinLODClamp = view.m_resourceMinLODClamp;
            }
          }
          else
          {
            if (!isArray)
            {
              srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
              srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
              srvdesc.Texture2DMSArray.ArraySize = arraySize;
              srvdesc.Texture2DMSArray.FirstArraySlice = view.m_arraySlice;
            }
          }
        }
        else if (desc.dimension == FormatDimension::Texture3D)
        {
          srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
          srvdesc.Texture3D.MipLevels = mipLevels;
          srvdesc.Texture3D.MostDetailedMip = view.m_mostDetailedMip;
          srvdesc.Texture3D.ResourceMinLODClamp = view.m_resourceMinLODClamp;
        }
        else if (desc.dimension == FormatDimension::TextureCube)
        {
          if (!isArray)
          {
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvdesc.TextureCube.MipLevels = mipLevels;
            srvdesc.TextureCube.MostDetailedMip = view.m_mostDetailedMip;
            srvdesc.TextureCube.ResourceMinLODClamp = view.m_resourceMinLODClamp;
          }
          else
          {
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            srvdesc.TextureCubeArray.First2DArrayFace = view.m_first2DArrayFace;
            srvdesc.TextureCubeArray.MipLevels = mipLevels;
            srvdesc.TextureCubeArray.MostDetailedMip = view.m_mostDetailedMip;
            srvdesc.TextureCubeArray.ResourceMinLODClamp = view.m_resourceMinLODClamp;
          }
        }
        srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // when should this differ?
        return srvdesc;
      }

      D3D12_UNORDERED_ACCESS_VIEW_DESC getUAV(ResourceDescriptor& resDesc, ShaderViewDescriptor& view)
      {
        auto& desc = resDesc.desc;

        FormatType format = view.m_format;
        if (format == FormatType::Unknown)
          format = desc.format;

        bool isArray = desc.arraySize > 1;
        int arraySize = view.m_arraySize;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavdesc = {};
        uavdesc.Format = formatTodxFormat(format).view;

        if (desc.dimension == FormatDimension::Texture1D)
        {
          if (!isArray)
          {
            uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavdesc.Texture1D.MipSlice = view.m_mostDetailedMip;
          }
          else
          {
            uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavdesc.Texture1DArray.ArraySize = arraySize;
            uavdesc.Texture1DArray.FirstArraySlice = view.m_arraySlice;
            uavdesc.Texture1DArray.MipSlice = view.m_mostDetailedMip;
          }
        }
        else if (desc.dimension == FormatDimension::Texture2D)
        {
          if (!isArray)
          {
            uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavdesc.Texture2D.PlaneSlice = view.m_planeSlice;
            uavdesc.Texture2D.MipSlice = view.m_mostDetailedMip;
          }
          else
          {
            uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavdesc.Texture2DArray.PlaneSlice = view.m_planeSlice;
            uavdesc.Texture2DArray.ArraySize = arraySize;
            uavdesc.Texture2DArray.FirstArraySlice = view.m_arraySlice;
            uavdesc.Texture2DArray.MipSlice = view.m_mostDetailedMip;
          }
        }
        else if (desc.dimension == FormatDimension::Texture3D)
        {
          uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
          uavdesc.Texture3D.FirstWSlice = 0; // The zero-based index of the first depth slice to be accessed.
          uavdesc.Texture3D.MipSlice = view.m_mostDetailedMip; // The zero-based index of the first depth slice to be accessed.
          uavdesc.Texture3D.WSize = 0; //The number of depth slices.
        }
        else
        {
          F_ASSERT(false, "dimension not bothered to write yet");
        }

        return uavdesc;
      }

      D3D12_RENDER_TARGET_VIEW_DESC getRTV(ResourceDescriptor& resDesc, ShaderViewDescriptor& view)
      {
        auto& desc = resDesc.desc;

        FormatType format = view.m_format;
        if (format == FormatType::Unknown)
          format = desc.format;

        bool isArray = desc.arraySize > 1;
        int arraySize = view.m_arraySize;

        D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
        rtv.Format = formatTodxFormat(format).view;

        if (desc.dimension == FormatDimension::Texture1D)
        {
          if (!isArray)
          {
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtv.Texture1D.MipSlice = view.m_mostDetailedMip;
          }
          else
          {
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtv.Texture1DArray.ArraySize = arraySize;
            rtv.Texture1DArray.FirstArraySlice = view.m_arraySlice;
            rtv.Texture1DArray.MipSlice = view.m_mostDetailedMip;
          }
        }
        else if (desc.dimension == FormatDimension::Texture2D)
        {
          if (desc.msCount == 1)
          {
            if (!isArray)
            {
              rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
              rtv.Texture2D.PlaneSlice = view.m_planeSlice;
              rtv.Texture2D.MipSlice = view.m_mostDetailedMip;
            }
            else
            {
              rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
              rtv.Texture2DArray.ArraySize = arraySize;
              rtv.Texture2DArray.FirstArraySlice = view.m_arraySlice;
              rtv.Texture2DArray.MipSlice = view.m_mostDetailedMip;
              rtv.Texture2DArray.PlaneSlice = view.m_planeSlice;
            }
          }
          else
          {
            if (!isArray)
            {
              rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
              rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
              rtv.Texture2DMSArray.ArraySize = arraySize;
              rtv.Texture2DMSArray.FirstArraySlice = view.m_arraySlice;
            }
          }
        }
        else if (desc.dimension == FormatDimension::Texture3D)
        {
          rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
          rtv.Texture3D.FirstWSlice = 0; // The zero-based index of the first depth slice to be accessed.
          rtv.Texture3D.MipSlice = view.m_mostDetailedMip; // The zero-based index of the first depth slice to be accessed.
          rtv.Texture3D.WSize = 0; //The number of depth slices.
        }
        else
        {
          F_ASSERT(false, "dimension not bothered to write yet");
        }
        return rtv;
      }

      D3D12_DEPTH_STENCIL_VIEW_DESC getDSV(ResourceDescriptor& resDesc, ShaderViewDescriptor& view)
      {
        auto& desc = resDesc.desc;

        FormatType format = view.m_format;
        if (format == FormatType::Unknown)
          format = desc.format;

        bool isArray = desc.arraySize > 1;
        int arraySize = view.m_arraySize;

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = formatTodxFormat(format).view;

        if (desc.dimension == FormatDimension::Texture1D)
        {
          if (!isArray)
          {
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsv.Texture1D.MipSlice = view.m_mostDetailedMip;
          }
          else
          {
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsv.Texture1DArray.ArraySize = arraySize;
            dsv.Texture1DArray.FirstArraySlice = view.m_arraySlice;
            dsv.Texture1DArray.MipSlice = view.m_mostDetailedMip;
          }
        }
        else if (desc.dimension == FormatDimension::Texture2D)
        {
          if (desc.msCount == 1)
          {
            if (!isArray)
            {
              dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
              dsv.Texture2D.MipSlice = view.m_mostDetailedMip;
            }
            else
            {
              dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
              dsv.Texture2DArray.ArraySize = arraySize;
              dsv.Texture2DArray.FirstArraySlice = view.m_arraySlice;
              dsv.Texture2DArray.MipSlice = view.m_mostDetailedMip;
            }
          }
          else
          {
            if (!isArray)
            {
              dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
              dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
              dsv.Texture2DMSArray.ArraySize = arraySize;
              dsv.Texture2DMSArray.FirstArraySlice = view.m_arraySlice;
            }
          }
        }
        else
        {
          F_ASSERT(false, "dimension not bothered to write yet");
        }
        return dsv;
      }
    }
  }
}
#endif