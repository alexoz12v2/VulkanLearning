// every output in the previous stage of the pipeline coming to the fragment shader has to have a
// corresponding input declared as an "in" function parameter. if such function was a used-defined struct
// SV_TARGET is a semantic stating that the output of the function should match the render target format,
// which in our case is a 4D color value
struct VertexOut
{
	[[vk::location(0)]] float4 color : COLOR;
};

float4 main(VertexOut vsOut) : SV_TARGET
{
	return vsOut.color;
}
