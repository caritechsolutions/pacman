#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef ECOS_OS
double round(double x)
{
    return x >= 0.0 ? 
        (int64_t)(x + 0.5) : 
        (int64_t)(x - 0.5);
}

float roundf(float x)
{
    return (float)round((double)x);
}

float powf(float x, float y)
{
    return (float)pow((double)x, (double)y);
}

float tanhf(float x)
{
    return (float)tanh((double)x);
}

float atan2f(float x, float y)
{
    return (float)atan2((double)x, (double)y);
}

float log10f(float x)
{
    return (float)log10((double)x);
}

float ceilf(float x)
{
    return (float)ceil((double)x);
}

float tanf(float x)
{
    return (float)tan((double)x);
}

float atanf(float x)
{
    return (float)atan((double)x);
}

float floorf(float x)
{
    return (float)floor((double)x);
}

float expf(float x)
{
    return (float)exp((double)x);
}

float logf(float x)
{
    return (float)log((double)x);
}

double trunc(double x)
{
    return (double)(int64_t)x;
}

float truncf(float x)
{
    return (float)trunc((double)x);
}

float fabsf(float x)
{
    uint32_t bits;
    // 通过memcpy进行安全类型转换
    memcpy(&bits, &x, sizeof(bits));
    bits &= 0x7FFFFFFFUL; // 清除符号位
    memcpy(&x, &bits, sizeof(x));
    return x;
}

float copysignf(float x, float y)
{
    uint32_t x_bits, y_bits;
    // 安全获取位模式
    memcpy(&x_bits, &x, sizeof(x_bits));
    memcpy(&y_bits, &y, sizeof(y_bits));

    // 组合符号位
    x_bits = (y_bits & 0x80000000UL) | (x_bits & 0x7FFFFFFFUL);
    memcpy(&x, &x_bits, sizeof(x));
    return x;
}

float erff(float x)
{
    // 实现策略相同，系数需重新优化
    const float a1 =  0.254829592f;
    const float a2 = -0.284496736f;
    const float a3 =  1.421413741f;
    const float a4 = -1.453152027f;
    const float a5 =  1.061405429f;
    const float p  =  0.3275911f;

    float t = 1.0f / (1.0f + p * fabsf(x));
    return copysignf(1.0f - t*(a1 + t*(a2 + t*(a3 + t*(a4 + t*a5))))*expf(-x*x), x);
}

float erfcf(float x)
{
    return 1.0f - erff(x);
}


double nearbyint(double x)
{
    return (x >= 0.0) ?
        (int64_t)(x + 0.5) :
        (int64_t)(x - 0.5);
}

float nearbyintf(float x)
{
    return (float)nearbyint((double)x);
}

double hypot(double x, double y)
{
    // 使用64位版本实现
    double a = fabs(x), b = fabs(y);
    if (a < b) { double t = a; a = b; b = t; }

    // 缩放因子根据双精度范围调整
    const double SCALE_UP   = 0x1.0p+512;  // 2^512
    const double SCALE_DOWN = 0x1.0p-512;  // 2^-512

    if (a > SCALE_UP) {
        a *= SCALE_DOWN;
        b *= SCALE_DOWN;
    } else if (a < SCALE_DOWN) {
        a *= SCALE_UP;
        b *= SCALE_UP;
    }

    double ratio = b / a;
    return a * sqrt(1.0 + ratio * ratio);
}
#endif
