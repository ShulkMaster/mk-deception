#include "libmkparticle/vm.h"
#include "math/gxMath.h"

float frand(float);
unsigned int random(void);
double sqrt(double);

static inline float rnd_inverse_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } estimate;
    float product;
    float correction;

    if (!(0.0f < value)) {
        return 0.0f;
    }
    estimate.f = value;
    estimate.u = 0x5F375A00U - (estimate.u >> 1);
    product = estimate.f * (value * estimate.f);
    correction = 3.0f - product;
    return 0.0625f * estimate.f * correction *
        (12.0f - correction * (product * correction));
}

static inline void rnd_cross(PfxVec3* output, const PfxVec3* left,
                             const PfxVec3* right) {
    output->x = left->y * right->z - left->z * right->y;
    output->y = left->z * right->x - left->x * right->z;
    output->z = left->x * right->y - left->y * right->x;
}

static inline void rnd_normalize(PfxVec3* vector) {
    float inverse = rnd_inverse_sqrt(
        vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
    vector->x *= inverse;
    vector->y *= inverse;
    vector->z *= inverse;
}

static inline float rnd_sqrt_table(float value) {
    union {
        float f;
        unsigned int u;
    } estimate;
    unsigned int bits;

    if (!(0.0f < value)) {
        return 0.0f;
    }
    estimate.f = value;
    bits = estimate.u;
    estimate.u = (unsigned int)GXMathSqrtTable[(bits >> 10) & 0x3FFE] << 8;
    estimate.u |= (((bits & 0x7F800000U) + 0x3F800000U) >> 1) &
        0x7F800000U;
    return 0.5f * estimate.f *
        (3.0f - (estimate.f * estimate.f) / value);
}

float rnd_between(float minimum, float maximum) {
    if (maximum > minimum) {
        return minimum + (maximum - minimum) * frand(1.0f);
    }
    return maximum + (minimum - maximum) * frand(1.0f);
}

int rnd_int(unsigned int maximum) {
    return random() % maximum;
}

void rnd_line_1i(int minimum, int maximum, int* output) {
    *output = minimum + (int)((maximum - minimum + 1) * frand(1.0f));
}

void rnd_sphere(PfxVec3* output, const PfxVec3* origin, int quadratic_radius,
                float minimum_radius, float maximum_radius) {
    float radius;
    float x;
    float y;
    float z;
    float scale;

    if (quadratic_radius) {
        float random_value = frand(1.0f);
        radius = minimum_radius + random_value * random_value *
            (maximum_radius - minimum_radius);
    } else {
        radius = rnd_between(minimum_radius, maximum_radius);
    }
    x = rnd_between(-1.0f, 1.0f);
    y = rnd_between(-1.0f, 1.0f);
    z = rnd_between(-1.0f, 1.0f);
    scale = radius / (float)sqrt(x * x + y * y + z * z);
    output->x = origin->x + x * scale;
    output->y = origin->y + y * scale;
    output->z = origin->z + z * scale;
}

void rnd_point_in_cylinder(PfxVec3* output, const PfxVec3* axis,
                           float radial_center, float radial_spread,
                           float axial_center, float axial_spread) {
    PfxVec3 random_vector;
    PfxVec3 radial;
    PfxVec3 axial = *axis;
    float radial_distance = rnd_between(
        radial_center - radial_spread, radial_center + radial_spread);
    float axial_distance = rnd_between(
        axial_center - axial_spread, axial_center + axial_spread);

    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(&radial, axis, &random_vector);
    rnd_normalize(&radial);
    radial.x *= radial_distance;
    radial.y *= radial_distance;
    radial.z *= radial_distance;
    rnd_normalize(&axial);
    axial.x *= axial_distance;
    axial.y *= axial_distance;
    axial.z *= axial_distance;
    output->x = axial.x + radial.x;
    output->y = axial.y + radial.y;
    output->z = axial.z + radial.z;
}

void rnd_point_in_disc(PfxVec3* output, const PfxVec3* axis,
                       float minimum_radius, float maximum_radius) {
    PfxVec3 random_vector;
    float radius;

    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(output, axis, &random_vector);
    rnd_normalize(output);
    radius = rnd_between(minimum_radius, maximum_radius);
    output->x *= radius;
    output->y *= radius;
    output->z *= radius;
}

void rnd_point_in_sphere_section(PfxVec3* output, const PfxVec3* axis,
                                 float radius, float radius_spread,
                                 float angle, float angle_spread) {
    PfxVec3 axial;
    PfxVec3 random_vector;
    PfxVec3 perpendicular;
    float cosine;
    float sine;
    float axial_length;

    gxMathCosSin(&cosine, &sine,
        angle + rnd_between(-angle_spread, angle_spread));
    axial.x = axis->x * (radius + rnd_between(-radius_spread, radius_spread));
    axial.y = axis->y * (radius + rnd_between(-radius_spread, radius_spread));
    axial.z = axis->z * (radius + rnd_between(-radius_spread, radius_spread));
    axial_length = rnd_sqrt_table(
        axial.x * axial.x + axial.y * axial.y + axial.z * axial.z);
    sine *= axial_length;
    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(&perpendicular, &random_vector, axis);
    rnd_normalize(&perpendicular);
    perpendicular.x *= sine;
    perpendicular.y *= sine;
    perpendicular.z *= sine;
    output->x = perpendicular.x + axial.x * cosine;
    output->y = perpendicular.y + axial.y * cosine;
    output->z = perpendicular.z + axial.z * cosine;
}

void rnd_vector_from_point(PfxVec3* output, const PfxVec3* start,
                           const PfxVec3* end, float minimum_length,
                           float length_range) {
    float length;

    output->x = end->x - start->x;
    output->y = end->y - start->y;
    output->z = end->z - start->z;
    rnd_normalize(output);
    length = rnd_between(minimum_length, minimum_length + length_range);
    output->x *= length;
    output->y *= length;
    output->z *= length;
}

void rnd_bend_vector(PfxVec3* vector, float angle, float angle_spread) {
    PfxVec3 random_vector;
    PfxVec3 perpendicular;
    float cosine;
    float sine;
    float length;

    gxMathCosSin(&cosine, &sine,
        angle + rnd_between(-angle_spread, angle_spread));
    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    length = rnd_sqrt_table(
        vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
    sine *= length;
    rnd_cross(&perpendicular, &random_vector, vector);
    rnd_normalize(&perpendicular);
    perpendicular.x *= sine;
    perpendicular.y *= sine;
    perpendicular.z *= sine;
    vector->x = perpendicular.x + vector->x * cosine;
    vector->y = perpendicular.y + vector->y * cosine;
    vector->z = perpendicular.z + vector->z * cosine;
}
