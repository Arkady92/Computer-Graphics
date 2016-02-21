#define INPUT_PATCH_SIZE 16
#define OUTPUT_PATCH_SIZE 16

Texture2D displacementMap : register(t0);
Texture2D colorMap : register(t1);
Texture2D normalMap : register(t2);
SamplerState domainTextureSampler : register(s0);
SamplerState pixelTextureSampler : register(s1);

cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

cbuffer cbCameraPos : register(b2) //Vertex Shader constant buffer slot 4
{
	float4 cameraPos;
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

cbuffer cbPatchIndex : register(b1) //Domain Shader constant buffer slot 1
{
	int index;
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
	float3 viewVec : TEXCOORD0;
	float3 lightVec : TEXCOORD1;
	float3 cameraPos: TEXCOORD2;
};

struct HSPatchOutput
{
	float edges[4] : SV_TessFactor;
	float inside[2] : SV_InsideTessFactor;
	float3 viewVec : TEXCOORD0;
	float3 lightVec : TEXCOORD1;
	float3 cameraPos: TEXCOORD2;
};

struct DSControlPoint
{
	float4 pos : POSITION;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 viewVec : TEXCOORD0;
	float3 lightVec : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
	float3 tangent : TEXCOORD3;
	float3 binormal : TEXCOORD4;
	float3 cameraPos: TEXCOORD5;
	float3 patchPos : TEXCOORD7;
};

static const float3 lightPosition = float3(0, 5, 0);
HSInput VS_Main(VSInput i)
{
	HSInput o;
	matrix worldView = mul(viewMatrix, worldMatrix);
	o.pos = mul(worldView, float4(i.pos, 1.0f));
	o.viewVec = normalize(-o.pos.xyz);
	o.lightVec = mul(viewMatrix, lightPosition);
	o.cameraPos = cameraPos;
	return o;
}

float CalculateLogarithmicFactor(float z)
{
	return (-16.0f * log10(z * 0.01f));
}

HSPatchOutput HS_PatchConstantFunc(InputPatch<HSInput, INPUT_PATCH_SIZE> i, uint patchId : SV_PrimitiveID)
{
	HSPatchOutput o;
	int edgeFactor = (int)(CalculateLogarithmicFactor(abs(i[patchId].cameraPos.z)));
	float diff = abs(-abs(i[patchId].cameraPos.z) - (i[patchId].pos.z));
	int interiorFactor = (int)(CalculateLogarithmicFactor(diff));

	o.edges[0] = o.edges[1] = o.edges[2] = o.edges[3] = edgeFactor + ETF;
	o.inside[0] = o.inside[1] = interiorFactor + ITF;
	o.viewVec = i[patchId].viewVec;
	o.lightVec = i[patchId].lightVec;
	o.cameraPos = i[patchId].cameraPos;
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

float4 CalculateBernsteinDerivativeFactor(float t)
{
	float negT = 1.0f - t;
	return float4(-3 * negT * negT, 3 * negT * negT - 6 * negT * t, 6 * negT * t - 3 * t * t, 3 * t * t);
}

float3 CalculatePatchPosition(const OutputPatch<DSControlPoint, OUTPUT_PATCH_SIZE> input, float4 U, float4 V)
{
	float3 patchPos = float3(0.0f, 0.0f, 0.0f);
	patchPos  = V.x * ( input[0].pos * U.x +  input[1].pos * U.y +  input[2].pos * U.z +  input[3].pos * U.w);
	patchPos += V.y * ( input[4].pos * U.x +  input[5].pos * U.y +  input[6].pos * U.z +  input[7].pos * U.w);
	patchPos += V.z * ( input[8].pos * U.x +  input[9].pos * U.y + input[10].pos * U.z + input[11].pos * U.w);
	patchPos += V.w * (input[12].pos * U.x + input[13].pos * U.y + input[14].pos * U.z + input[15].pos * U.w);
	return patchPos;
}

float CalculateMipLevel(float z)
{
	return 6.0f - log2(CalculateLogarithmicFactor(z));
}

[domain("quad")]
PSInput DS_Main(HSPatchOutput i, float2 UV : SV_DomainLocation, const OutputPatch<DSControlPoint, OUTPUT_PATCH_SIZE> input)
{
	PSInput o;
	float4 U = CalculateBernsteinFactor(UV.x);
	float4 V = CalculateBernsteinFactor(UV.y);
	float4 dU = CalculateBernsteinDerivativeFactor(UV.x);
	float4 dV = CalculateBernsteinDerivativeFactor(UV.y);

	float3 patchPos = CalculatePatchPosition(input, U, V);
	float3 dUPos = CalculatePatchPosition(input, dU, V);
	float3 dVPos = CalculatePatchPosition(input, U, dV);
	float3 norm = normalize(cross(dUPos, dVPos));

	float mipLevel = 6.0f - log2(CalculateLogarithmicFactor(patchPos.z));
	int indexX = index % 4;
	int indexY = index / 4;
	float shift = 0.25;
	float2 texcoord = float2((UV.x / 4.0f) + shift * indexX, ((1.0f - UV.y) / 4.0f) + shift * indexY);
	float h = displacementMap.SampleLevel(domainTextureSampler, texcoord, mipLevel).x;

	o.pos = mul(projMatrix, float4(patchPos + h * 0.4f * norm, 1.0f));
	o.patchPos = patchPos;
	o.viewVec = normalize(-patchPos.xyz);
	o.lightVec = i.lightVec;
	o.norm = norm;
	o.tangent = dUPos;
	o.binormal = dVPos;
	o.texcoord = texcoord;
	o.cameraPos = i.cameraPos;
	return o;
}

static const float3 ambientColor = float3(0.3f, 0.3f, 0.3f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float3 kd = 0.7, ks = 1.0f, m = 100.0f;

float4 PS_Main(PSInput i) : SV_TARGET
{
	float3 viewVec = normalize(i.viewVec);
	float3 lightVec = normalize(i.lightVec - i.patchPos);
	float3 texNorm = normalize(normalMap.Sample(pixelTextureSampler, i.texcoord));
	float3 normal = normalize(texNorm.x * i.tangent + texNorm.y * i.binormal + texNorm.z * i.norm);
	float3 halfVec = normalize(viewVec + lightVec);

	float3 texColor = colorMap.Sample(pixelTextureSampler, i.texcoord);
	float3 color = texColor * ambientColor;
	color += lightColor * texColor * kd * saturate(dot(normal, lightVec)) + lightColor * ks * pow(saturate(dot(normal, halfVec)), m);
	return float4(saturate(color), surfaceColor.a);
}