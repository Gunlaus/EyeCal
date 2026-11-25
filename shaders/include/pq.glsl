#ifndef EYECAL_PQ_GLSL
#define EYECAL_PQ_GLSL

// SMPTE ST 2084 / ITU-R BT.2100 inverse EOTF. Input is cd/m².
float pqEncode(float nits)
{
    float p = pow(clamp(nits / 10000.0, 0.0, 1.0), 2610.0 / 16384.0);
    return pow((3424.0 / 4096.0 + (2413.0 / 128.0) * p) /
               (1.0 + (2392.0 / 128.0) * p), 2523.0 / 32.0);
}

#endif
