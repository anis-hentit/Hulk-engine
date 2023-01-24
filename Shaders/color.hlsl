
// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.s
#include "LightingUtil.hlsl"



//diffuse texture
Texture2D    gDiffuseMap : register(t0);

SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWrap : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWrap : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);

//per renderpass constant buffer
cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerPassPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPerPassPad2;

	Light gLights[MaxLights];
};

// per object constant buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};


cbuffer MaterialCB : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexCord : TEXTCOR;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;// keep vertex world position to calculate distance between eye and the points on the surface
	float3 NormalW : NORMAL;
	float2 TexCord : TEXTCOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	
	vout.PosW = posW.xyz; // get the world pos for eye-point distance calculation
   
	
	//normals are vectors so they are not affected by translation (so remove translation row) 4x4 -> 3x3
	// Assumes uniform scaling; otherwise, need to use inverse-transpose of world matrix.
	vout.NormalW = mul(vin.NormalL,(float3x3)gWorld);
	

	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	// this outputs texCoord per vertex for interpolation in rasterizer
	float4 texC = mul(float4(vin.TexCord, 0.0f, 1.0f), gTexTransform);
	vout.TexCord = mul(texC, gMatTransform).xy;

	
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);
	
    float4 DiffuseAlbedo = gDiffuseMap.Sample(gSamAnisotropicWrap, pin.TexCord) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST

		// Discard pixel if texture alpha < 0.1. We do this test as soon
		// as possible  in the shader so that we can potentially exit the
		//shader early, thereby skipping the rest of the shader code.
	clip(DiffuseAlbedo.a - 0.1);
	

#endif


	

	// Vector from point being lit to eye. 
	float3 toEyeW = gEyePosW - pin.PosW;

	//distance from point to eye
	float disToEye = length(toEyeW);

	toEyeW /= disToEye;	//normalize to eye vector for lighting computations

	// Indirect lighting.
	float4 ambient = gAmbientLight * DiffuseAlbedo;//component wise multiplication so ambient (x1*x2,y1*y2,z1*z2,w1*w2)

	const float shininess = 1.0f - gRoughness;
	Material mat = { DiffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;

	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);
											
	
	float4 litColor = ambient + directLight;
	
#ifdef FOG
	float fogAmount = saturate((disToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount); // note : we can also clear the back buffer with fogcolor for the fog to show better 
#endif

	// Common convention to take alpha from diffuse material.
	litColor.a = DiffuseAlbedo.a;
	
	return litColor;
	
}





