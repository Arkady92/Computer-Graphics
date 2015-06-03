Texture2D colorMap1 : register(t0);
Texture2D colorMap2 : register(t1);
SamplerState colorSampler : register(s0);
SamplerCUBE colorSampler2 : register(s1);
static const float transparency = 0.35f;

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

cbuffer cbTexture1Transform : register(b3)
{
	matrix texMatrix1;
};

cbuffer cbTexture2Transform : register(b4)
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
	float2 tex1: TEXCOORD0;
	float2 tex2: TEXCOORD1;
	float3 viewVec : TEXCOORD2;
	float3 lightVec : TEXCOORD3;
};

static const float3 lightPos = float3(-5.0f, 5.0f, -5.0f);


float3 ray_to_texcoord(float3 p, float3 dir)
{
	float dx, dy, dz;
	dx = -p.x / dir.x + abs(1 / dir.x);
	dy = -p.y / dir.y + abs(1 / dir.y);
	dz = -p.z / dir.z + abs(1 / dir.z);
	return p + min(dx, min(dy, dz)) * dir;
}

PSInput VS_Main(VSInput i)
{
	PSInput o = (PSInput)0;
	o.pos = float4(i.pos, 1.0f);
	matrix worldView = mul(viewMatrix, worldMatrix);
	float4 viewPos = float4(i.pos, 1.0f);
		viewPos = mul(worldView, viewPos);

	o.tex1 = mul(texMatrix1, o.pos).xy;
	o.tex2 = mul(texMatrix2, o.pos).xy;

	o.pos = mul(worldMatrix, o.pos);
	o.pos = mul(viewMatrix, o.pos);
	o.pos = mul(projMatrix, o.pos);

	o.viewVec = normalize(-viewPos.xyz);
	o.lightVec = normalize((mul(viewMatrix, lightPos) - viewPos).xyz);

	return o;
}

static const float3 ambientColor = float3(0.3f, 0.3f, 0.3f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float3 kd = 0.7, ks = 1.0f, m = 100.0f;

float4 PS_Main(PSInput i) : SV_TARGET
{
	float3 normal = normalize(colorMap2.Sample(colorSampler, i.tex2).xyz);
	float3 viewVec = normalize(i.viewVec);
	float3 lightVec = normalize(i.lightVec);
	float3 halfVec = normalize(viewVec + lightVec);
	float3 ViewRefl = reflect(i.viewVec, normal);
	float3 ViewRefr = normalize(-i.viewVec - 1.5 * normal);
	ViewRefr = ray_to_texcoord(i.pos, ViewRefr);
	ViewRefl = ray_to_texcoord(i.pos, ViewRefl);
	float4 refl = colorMap1.Sample(colorSampler2, ViewRefl);
		float4 refr = float4(0.7, 1.0, 0.8, 0.1) * colorMap1.Sample(colorSampler2, ViewRefr);

		float3 color = lerp(refr, refl, 0.3f + 0.7 * abs(dot(-i.viewVec, float3(0, 1, 0))));
		color += lightColor * colorMap1.Sample(colorSampler, i.tex1).xyz * kd * saturate(dot(normal, lightVec)) +
		lightColor * ks * pow(saturate(dot(normal, halfVec)), m);
	return float4(saturate(color), colorMap1.Sample(colorSampler, i.tex1).a / 2);
}