cbuffer cbWorld : register(b0)
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1)
{
	matrix viewMatrix;
};

cbuffer cbProj : register(b2)
{
	matrix projMatrix;
};

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

struct VSInput
{
	float3 pos : POSITION;
};

struct PSInput
{
	float4 pos : SV_POSITION;
};

PSInput VS_Main(VSInput i)
{
	PSInput o = (PSInput)0;
	matrix worldView = mul(viewMatrix, worldMatrix);
	float4 viewPos = float4(i.pos, 1.0f);
	viewPos = mul(worldView, viewPos);
	o.pos = mul(projMatrix, viewPos);
	return o;
}


float4 PS_Main(PSInput i) : SV_TARGET
{
	return surfaceColor;
}