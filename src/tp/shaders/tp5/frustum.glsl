
//! \file compute_buffer.glsl exemple compute shader + buffers

#version 430

#ifdef COMPUTE_SHADER

layout( std430, binding= 0 ) buffer inputData
{
    int dataIn[];
};

layout( std430, binding= 1 ) buffer outputData
{
    int dataOut[];
};

layout( local_size_x=256 ) in;

bool isVisibleInObjectSpace(Point pMin, Point pMax,vec4 boundingBox[8]) {

    vec4 v000 = boundingBox[0]; vec4 v001 = boundingBox[1];
    vec4 v010 = boundingBox[2]; vec4 v011 = boundingBox[3];
    vec4 v100 = boundingBox[4]; vec4 v101 = boundingBox[5];
    vec4 v110 = boundingBox[6]; vec4 v111 = boundingBox[7];

    if(v000.x > pMax.x && v001.x > pMax.x && v010.x > pMax.x && v011.x > pMax.x &&
    v100.x > pMax.x && v101.x > pMax.x && v110.x > pMax.x && v111.x > pMax.x ) {
        return false;
    }

    if(v000.x < pMin.x && v001.x < pMin.x && v010.x < pMin.x && v011.x < pMin.x &&
    v100.x < pMin.x && v101.x < pMin.x && v110.x < pMin.x && v111.x < pMin.x ) {
        return false;
    }

    if(v000.y > pMax.y && v001.y > pMax.y && v010.y > pMax.y && v011.y > pMax.y &&
    v100.y > pMax.y && v101.y > pMax.y && v110.y > pMax.y && v111.y > pMax.y ) {
        return false;
    }

    if(v000.y < pMin.y && v001.y < pMin.y && v010.y < pMin.y && v011.y < pMin.y &&
    v100.y < pMin.y && v101.y < pMin.y && v110.y < pMin.y && v111.y < pMin.y ) {
        return false;
    }

    if(v000.z > pMax.z && v001.z > pMax.z && v010.z > pMax.z && v011.z > pMax.z &&
    v100.z > pMax.z && v101.z > pMax.z && v110.z > pMax.z && v111.z > pMax.z ) {
        return false;
    }

    if(v000.z < pMin.z && v001.z < pMin.z && v010.z < pMin.z && v011.z < pMin.z &&
    v100.z < pMin.z && v101.z < pMin.z && v110.z < pMin.z && v111.z < pMin.z ) {
        return false;
    }

    return true;
}

bool cull(uint ID)
{
    return a[ID] > 0;
}

uniform int value;

void main( )
{
    uint ID= gl_GlobalInvocationID.x;
    if(ID < dataIn.length()) {
        dataOut[ID]= cull(ID);
    }
}

#endif
