// all input declared while specifying vertex buffers are not declared as layout(location = ) in ... like in GLSL
// but passed as input to the entry point function in HLSL
// only though difference to get used to is that HLSL DOES NOT HAVE POINTERS/REFERENCES. We declare "out" parameters as a type qualifier
// each function parameter can have associated a SEMANTIC "out float4 oPosH : SV_POSITION". Semantics are used to MAP THE CORRENSPONDING VERTEX SHADER OUTPUT TO THE INPUT
// OF THE NEXT STAGE. Semantics are used to specify usage of vertex attributes. Output parameters also have semantics. "SV_" in the semantic name means that it is a SystemValue.
// In particular, SV_POSITION is used to denote the required output of the vertex shader stage onto the next stage, i.e. vertex position in Homogeneous coordinates, screen space.
// a complete list for semantics for each vertex stage can be found in microsoft's HLSL documentation. There is much more we can attach to a variable.
struct VertexOut
{
	float4 posH  : SV_Position;
	float4 color : COLOR;
};

// while in GLSL you can access wherever you want in vertex shader gl_VertexIndex, here you need to declare a nonout parameter to the entry point which is going to be of type uint and semantics SV_VertexID

// equivalent to "VertexOut main() {..., return vsOut;}" where vsOut is created in the stack of the function
void main(uint vID : SV_VertexID, out VertexOut vsOut)
{
	// some system values are not semantics but pre generated values, such as SV_InstanceID(per instance index), SV_IsFrontFace, SV_PrimitiveID,...
	// we will use the system value SV_VertexID to identify which vertex we are drawing.
	vector <float, 3> positions[3] = {
		float3(0.f, -0.2f, 0.f),
		float3(0.4f, 0.2f, 0.f),
		float3(-0.4f, 0.2f,0.f)
	};
	
	vsOut.posH = float4(positions[vID], 1.f);
	vsOut.color = float4(0.7f, 0.3f, 0.1f, 1.f);
}
