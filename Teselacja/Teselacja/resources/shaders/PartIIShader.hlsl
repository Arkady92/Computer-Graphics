#define INPUT_PATCH_SIZE 16
#define OUTPUT_PATCH_SIZE 16


cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

cbuffer cbEdgeTF : register(b0) //Hull Shader constant buffer slot 0
{
	float ETF;
};
cbuffer cbInteriorTF : register(b1) //Hull Shader constant buffer slot 1
{
	float ITF;
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
	float edges[4] : SV_TessFactor;
	float inside[2] : SV_InsideTessFactor;
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

HSPatchOutput HS_PatchConstantFunc(InputPatch<HSInput, INPUT_PATCH_SIZE> i, uint patchId : SV_PrimitiveID)
{
	HSPatchOutput o;
	o.edges[0] = o.edges[1] = o.edges[2] = o.edges[3] = ETF;
	o.inside[0] = o.inside[1] = ITF;
	return o;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("HS_PatchConstantFunc")]
DSControlPoint HS_Main(InputPatch<HSInput, INPUT_PATCH_SIZE> i, uint id : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
	DSControlPoint o;
	o.pos = i[id].pos;
	return o;
}

float4 CalculateBernsteinFactor(float t)
{
	float negT = 1.0f - t;
	return float4(negT * negT * negT, 3.0f * negT * negT * t, 3.0f * negT * t * t, t * t * t);
}

[domain("quad")]
PSInput DS_Main(HSPatchOutput i, float2 UV : SV_DomainLocation, const OutputPatch<DSControlPoint, OUTPUT_PATCH_SIZE> input)
{
	PSInput o;
	float4 U = CalculateBernsteinFactor(UV.x);
	float4 V = CalculateBernsteinFactor(UV.y);
	float4 patchPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
	patchPos  = V.x * ( input[0].pos * U.x +  input[1].pos * U.y +  input[2].pos * U.z +  input[3].pos * U.w);
	patchPos += V.y * ( input[4].pos * U.x +  input[5].pos * U.y +  input[6].pos * U.z +  input[7].pos * U.w);
	patchPos += V.z * ( input[8].pos * U.x +  input[9].pos * U.y + input[10].pos * U.z + input[11].pos * U.w);
	patchPos += V.w * (input[12].pos * U.x + input[13].pos * U.y + input[14].pos * U.z + input[15].pos * U.w);
	patchPos.w = 1.0f;
	o.pos = mul(projMatrix, patchPos);
	return o;
}

float4 PS_Main(PSInput i) : SV_TARGET
{
	return surfaceColor;
}