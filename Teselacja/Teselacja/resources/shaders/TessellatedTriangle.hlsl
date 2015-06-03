#define INPUT_PATCH_SIZE 3
#define OUTPUT_PATCH_SIZE 3


cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

cbuffer cbProj : register(b0) //Domain Shader constant buffer slot 0
{
	matrix projMatrix;
};

cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0
{
	float4 surfaceColor;
};

struct VSInput
{
	float3 pos : POSITION;
};

struct HSInput
{
	float4 pos : POSITION;
};

struct HSPatchOutput
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DSControlPoint
{
	float4 pos : POSITION;
};

struct PSInput
{
	float4 pos : SV_POSITION;
};

HSInput VS_Main(VSInput i)
{
	HSInput o;
	matrix worldView = mul(viewMatrix, worldMatrix);
	o.pos = mul(worldView, float4(i.pos, 1.0f));
	return o;
}

HSPatchOutput HS_Patch(InputPatch<HSInput, INPUT_PATCH_SIZE> ip, uint patchId : SV_PrimitiveID)
{
	HSPatchOutput o;
	o.edges[0] = o.edges[1] = o.edges[2] = 8.0f;
	o.inside = 8.0f;
	return o;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("HS_Patch")]
DSControlPoint HS_Main(InputPatch<HSInput, INPUT_PATCH_SIZE> ip, uint i : SV_OutputControlPointID,
					  uint patchID : SV_PrimitiveID)
{
	DSControlPoint o;
	o.pos = ip[i].pos;
	return o;
}

[domain("tri")]
PSInput DS_Main(HSPatchOutput factors, float3 uvw : SV_DomainLocation,
			   const OutputPatch<DSControlPoint, OUTPUT_PATCH_SIZE> input)
{
	PSInput o;
	float4 viewPos = input[0].pos * uvw.x + input[1].pos * uvw.y + input[2].pos * uvw.z;
	o.pos = mul(projMatrix, viewPos);
	return o;
}

float4 PS_Main(PSInput i) : SV_TARGET
{
	return surfaceColor;
}