Texture2D colorMap : register(t0);
Texture2D colorMap2 : register(t1);
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

cbuffer cbTextureTransform2 : register(b4)
{
	matrix texMatrix2;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
	float2 tex2: TEXCOORD1;
};

PSInput VS_Main(VSInput i)
{
	PSInput o = (PSInput)0;
	o.pos = float4(i.pos, 1.0f);

	o.tex = mul(texMatrix, o.pos).xy;
	o.tex2 = mul(texMatrix2, o.pos).xy;

	o.pos = mul(worldMatrix, o.pos);
	o.pos = mul(viewMatrix, o.pos);
	o.pos = mul(projMatrix, o.pos);
	
	return o;
}

float4 PS_Main(PSInput i) : SV_TARGET
{
	float4 bricks = colorMap.Sample(colorSampler, i.tex); 
	float4 poster = colorMap2.Sample(colorSampler, i.tex2);
	return poster * poster.a + bricks * (1 - poster.a);
}