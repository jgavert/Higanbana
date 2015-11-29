

struct imGuiVertices
{
    float2 pos;
    float4 col;
    float2 uv;
};

struct imGuiPSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};
