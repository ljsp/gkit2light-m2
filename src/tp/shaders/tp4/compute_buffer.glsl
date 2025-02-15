
//! \file compute_buffer.glsl exemple compute shader + buffers

#version 430

#ifdef COMPUTE_SHADER

layout( std430, binding= 0 ) buffer inputData
{
	int a[];
};

layout( std430, binding= 1 ) buffer outputData
{
	int b[];
};

layout( local_size_x=256 ) in;

uniform int value;

void main( )
{
	uint ID= gl_GlobalInvocationID.x;
	if(ID < a.length()) {
		b[ID]= a[ID] + value;
	}
}

#endif
