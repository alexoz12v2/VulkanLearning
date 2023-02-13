// all input declared while specifying vertex buffers are not declared as layout(location = ) in ... like in GLSL
// but passed as input to the entry point function in HLSL
// only though difference to get used to is that HLSL DOES NOT HAVE POINTERS/REFERENCES. We declare "out" parameters as a type qualifier
// each function parameter can have associated a SEMANTIC "out float4 oPosH : SV_POSITION". Semantics are used to MAP THE CORRENSPONDING VERTEX SHADER OUTPUT TO THE INPUT
// OF THE NEXT STAGE. Semantics are used to specify usage of vertex attributes. Output parameters also have semantics. "SV_" in the semantic name means that it is a SystemValue.
// In particular, SV_POSITION is used to denote the required output of the vertex shader stage onto the next stage, i.e. vertex position in Homogeneous coordinates, screen space.
// a complete list for semantics for each vertex stage can be found in microsoft's HLSL documentation. There is much more we can attach to a variable.

// location attribute can be attached to whatever semantics can be attached (struct fields, global variables, return of a function)
// binding attribute can be attached to global variables. First number is binding number, second is descriptor set id

struct VertexOut
{
	float4 posH  : SV_Position;
	[[vk::location(0)]] float4 color : COLOR;
};
struct VertexIn
{
	[[vk::location(0)]] float3 pos : POSITION;
	[[vk::location(1)]] float3 col : COLOR;
};

struct UBO
{
	float4x4 mvp; // todo change into model view and projection, or all 3 separate
};

// constant buffers are the equivalent of uniform buffers, space0 is the binding declaration? or set declaration?
// cbuffer cbuf : register(b0, space0) { UBO ubo; } // register(b0, space0) in directx has a meaning of which i'm not aware, but in vulkan means set 0(second 0), binding 0. Alternative to location specification
[[vk::binding(0,0)]] cbuffer cbuf { UBO ubo; }


// while in GLSL you can access wherever you want in vertex shader gl_VertexIndex, here you need to declare a nonout parameter to the entry point which is going to be of type uint and semantics SV_VertexID

// equivalent to "VertexOut main() {..., return vsOut;}" where vsOut is created in the stack of the function
void main(VertexIn input, uint vID : SV_VertexID, out VertexOut vsOut)
{
	// some system values are not semantics but pre generated values, such as SV_InstanceID(per instance index), SV_IsFrontFace, SV_PrimitiveID,...
	// we will use the system value SV_VertexID to identify which vertex we are drawing.
	// vector <float, 3> positions[3] = {
	// 	float3(0.f, -0.2f, 0.f),
	// 	float3(0.4f, 0.2f, 0.f),
	// 	float3(-0.4f, 0.2f,0.f)
	// };
	
	// vsOut.posH = float4(positions[vID], 1.f);
	vsOut.posH = mul(ubo.mvp, float4(input.pos, 1.f));
	vsOut.color = float4(input.col, 1.f);
}
