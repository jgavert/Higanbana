#define MyRS4 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
        "RootConstants(num32BitConstants=1, b1), " \
        "CBV(b0), " \
        "SRV(t0), " \
        "UAV(u0), " \
        "DescriptorTable( SRV(t1, numDescriptors = 60)), " \
        "DescriptorTable( UAV(u1, numDescriptors = 60)) " 

struct RootConstants
{
  uint texIndex;
};
