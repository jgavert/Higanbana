#pragma once
#include <windows.h>
#include <vector>

namespace higanbana
{
struct CpuCore
{
  std::vector<int> logicalCores;
};

struct L3CacheCpuGroup
{
  uint64_t mask;
  std::vector<CpuCore> cores;
};

struct Numa
{
  DWORD number = 0;
  WORD processor = 0;
  size_t cores = 0;
  size_t threads = 0;
  std::vector<L3CacheCpuGroup> coreGroups;
};

class SystemCpuInfo {
  public:
  std::vector<Numa> numas;
  SystemCpuInfo() {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION info = nullptr;
    DWORD infoLen = 0;
    if (!GetLogicalProcessorInformation(info, &infoLen)){
      auto error = GetLastError();
      if (error == ERROR_INSUFFICIENT_BUFFER) {
        info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(infoLen));
        if (GetLogicalProcessorInformation(info, &infoLen)) {
          DWORD byteOffset = 0;
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = info;
          std::vector<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION> ptrs;
          while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= infoLen) {
            ptrs.push_back(ptr);
            switch(ptr->Relationship) {
              case RelationNumaNode:
              {
                GROUP_AFFINITY affi;
                bool woot = GetNumaNodeProcessorMaskEx(ptr->NumaNode.NodeNumber, &affi);
                numas.push_back({ptr->NumaNode.NodeNumber, affi.Group});
                break;
              }
              default:
                break;
            }
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr++;
          }

          for (auto& numa : numas) {
            for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pt : ptrs) {
              switch(pt->Relationship) {
                case RelationProcessorCore:
                {
                  break;
                }
                case RelationCache:
                {
                  if (pt->Cache.Level == 3) {
                    L3CacheCpuGroup group{};
                    group.mask = static_cast<uint64_t>(pt->ProcessorMask);
                    numa.coreGroups.push_back(group);
                  }
                  break;
                }
                case RelationProcessorPackage:
                {
                  break;
                }
                default:
                  break;
              }
            }
            for (auto& l3ca : numa.coreGroups)
              for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pt : ptrs) {
                auto res = l3ca.mask & pt->ProcessorMask;
                if (res != pt->ProcessorMask)
                  continue;
                switch(pt->Relationship) {
                  case RelationProcessorCore:
                  {
                    CpuCore core{};
                    auto mask = pt->ProcessorMask;
                    numa.cores++;
                    while(mask != 0) {
                      unsigned long index = -1;
                      if (_BitScanForward(&index, mask)!=0) {
                        core.logicalCores.push_back(static_cast<int>(index));
                        numa.threads++;
                        unsigned long unsetMask = 1 << index;
                        mask ^= unsetMask;
                      }
                      else{
                        break;
                      }
                    }
                    l3ca.cores.push_back(core);
                    break;
                  }
                  default:
                    break;
                }
              }
          }
        }
        free(info);
      }
    }
  }
};
};