samplerCUBE envmap;

struct VS_INPUT 
{
   float4 Position : POSITION0;
};

struct VS_OUTPUT 
{
   float4 Position : POSITION0;
   float3 TexCoord : TEXCOORD0;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;
   Output.TexCoord = Input.Position.xyz;
   Input.Position.xyz *= 400.0;
   Output.Position = mul( Input.Position, matViewProjection );
   return( Output );
}

float4 ps_main(float3 TexCoord : TEXCOORD0) : COLOR0
{   
   return texCUBE(envmap, TexCoord);
}