
//! \file compute_buffer.glsl exemple compute shader + buffers

#version 430

#ifdef COMPUTE_SHADER

layout( std430, binding= 0 ) readonly buffer inputData
{
    int dataIn[];
};

layout( std430, binding= 1 ) writeonly buffer outputData
{
    int dataOut[];
};

layout( local_size_x=256 ) in;

uniform int value;

void main( )
{
    uint ID= gl_GlobalInvocationID.x;
    if(ID < dataIn.length())
        dataOut[ID]= dataIn[ID] + value;
}

#endif
