Texture2D colorMap : register(t0);
Texture2D colorMapA : register(t1);
SamplerState colorSampler : register(s0);

cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2
{
	matrix projMatrix;
};

cbuffer cbTextureTransform : register(b3)
{
	matrix texMatrix;
};

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
	float u : TEXCOORD0;
	float v : TEXCOORD1;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
	float4 norm : TEXCOORD1;
	float4 pos2	: TEXCOORD2;
	float3 viewVec : TEXCOORD3;
	float3 lightVec : TEXCOORD4;
};

static const float3 lightPos = float3(-5.0f, 5.0f, -5.0f);

PSInput VS_Main(VSInput i)
{
	PSInput o = (PSInput)0;
	o.pos = float4(i.pos, 1.0f);
	float4 viewPos = float4(i.pos, 1.0f);

	o.tex = float2(i.u, i.v);

	o.pos = mul(worldMatrix, o.pos);
	o.pos = mul(viewMatrix, o.pos);
	o.pos = mul(projMatrix, o.pos);

	o.pos2 = o.pos;
	o.norm = mul(worldMatrix, -i.norm);
	o.norm = mul(viewMatrix, o.norm);
	o.norm = mul(projMatrix, o.norm);

	o.viewVec = normalize(-viewPos.xyz);
	o.lightVec = normalize((mul(viewMatrix, lightPos) - viewPos).xyz);

	return o;
}

float4 PS_Main(PSInput i) : SV_TARGET
{
	float4 col = colorMap.Sample(colorSampler, i.tex);

	float4 tempPos = float4(i.pos2.xyz, 1);
	float3 worldSpacePos = mul(tempPos, (float4x3)worldMatrix);
	float3 vertToEye = normalize(i.viewVec - worldSpacePos);
	float3 halfAngle = normalize(vertToEye + i.lightVec);
	float2 uv;
	uv.x = clamp(max(dot(i.lightVec, i.norm), 0.0), 0.0, 1.0);
	uv.y = clamp(max(dot(halfAngle, i.norm), 0.0), 0.0, 1.0);

	float4 d = colorMapA.Sample(colorSampler, uv);
	float4 res;
	res.xyz = (col.xyz * d.aaa) * 2.0;
	res.w = 1.0;
	return res;
}