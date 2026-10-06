#include "libmkparticle/random.h"
#include "libmkparticle/vm.h"
#include "math/gxMath.h"
#include "runtime/utils.h"
#include "fdlibm.h"

union RandomFloatBits {
    float f;
    unsigned int u;
};

static inline float rnd_inverse_sqrt(float value) {
    union RandomFloatBits estimate;
    float product;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    estimate.f = value;
    estimate.u = 0x5F375A00U - (estimate.u >> 1);
    product = estimate.f * (value * estimate.f);
    correction = 3.0f - product;
    return 0.0625f * estimate.f * correction *
        (12.0f - correction * (product * correction));
}

static inline void rnd_cross(PfxVec3* const output, const PfxVec3* const left,
                             const PfxVec3* const right) {
    output->x = left->y * right->z - left->z * right->y;
    output->y = left->z * right->x - left->x * right->z;
    output->z = left->x * right->y - left->y * right->x;
}

static inline float rnd_sqrt_table(float value) {
    union RandomFloatBits input;
    union RandomFloatBits estimate;
    unsigned int bits;
    float result;

    input.f = value;
    if (value <= 0.0f) {
        result = 0.0f;
    } else {
        bits = input.u;
        estimate.u = (unsigned int)GXMathSqrtTable[(bits >> 11) & 0x1FFF] << 8;
        estimate.u |= (((bits & 0x7F800000U) + 0x3F800000U) >> 1) &
            0x7F800000U;
        result = 0.5f * (estimate.f *
            (3.0f - (estimate.f * estimate.f) / value));
    }
    return result;
}

static inline void rnd_normalize(PfxVec3* const output, const PfxVec3* const input) {
    float inverse = rnd_inverse_sqrt(
        input->x * input->x + input->y * input->y + input->z * input->z);
    output->x = input->x * inverse;
    output->y = input->y * inverse;
    output->z = input->z * inverse;
}

static inline float rnd_length(const PfxVec3* const vector) {
    return rnd_sqrt_table(
        vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
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

void rnd_sphere(PfxVec3* output, const PfxVec3* origin,
                float minimum_radius, float maximum_radius, int quadratic_radius) {
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
    x *= scale;
    y *= scale;
    z *= scale;
    output->x = origin->x + x;
    output->y = origin->y + y;
    output->z = origin->z + z;
}

void rnd_point_in_cylinder(PfxVec3* output, const PfxVec3* axis,
                           float radial_center, float radial_spread,
                           float axial_center, float axial_spread) {
    PfxVec3 random_vector;
    PfxVec3 radial;
    PfxVec3 axial;
    float radial_distance = rnd_between(
        radial_center - radial_spread, radial_center + radial_spread);
    float axial_distance = rnd_between(
        axial_center - axial_spread, axial_center + axial_spread);

    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(&radial, axis, &random_vector);
    rnd_normalize(&radial, &radial);
    radial.x *= radial_distance;
    radial.y *= radial_distance;
    radial.z *= radial_distance;
    rnd_normalize(&axial, axis);
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
    PfxVec3 perpendicular;
    float radius;

    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(&perpendicular, axis, &random_vector);
    rnd_normalize(&perpendicular, &perpendicular);
    radius = rnd_between(minimum_radius, maximum_radius);
    output->x = perpendicular.x * radius;
    output->y = perpendicular.y * radius;
    output->z = perpendicular.z * radius;
}

/* TODO: [near miss] 99.48%; vector temps stack-resident and CFG exact; normalize inverse/sine FPR coloring (retail f1, ours f3) remains. */
void rnd_point_in_sphere_section(PfxVec3* output, const PfxVec3* axis,
                                 float radius, float radius_spread,
                                 float angle, float angle_spread) {
    PfxVec3 perpendicular;
    PfxVec3 axial;
    PfxVec3 random_vector;
    float sine;
    float cosine;
    float axial_length;

    gxMathCosSin(&cosine, &sine,
        angle + rnd_between(-angle_spread, angle_spread));
    axial.x = axis->x * (radius + rnd_between(-radius_spread, radius_spread));
    axial.y = axis->y * (radius + rnd_between(-radius_spread, radius_spread));
    axial.z = axis->z * (radius + rnd_between(-radius_spread, radius_spread));
    axial_length = rnd_length(&axial);
    sine *= axial_length;
    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    rnd_cross(&perpendicular, &random_vector, axis);
    rnd_normalize(&perpendicular, &perpendicular);
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
    PfxVec3 direction;
    float length;

    direction.x = end->x - start->x;
    direction.y = end->y - start->y;
    direction.z = end->z - start->z;
    rnd_normalize(&direction, &direction);
    length = rnd_between(minimum_length, minimum_length + length_range);
    direction.x *= length;
    direction.y *= length;
    direction.z *= length;
    output->x = direction.x;
    output->y = direction.y;
    output->z = direction.z;
}

/* TODO: [near miss] 99.36%; vector temps stack-resident and CFG exact; normalize inverse/sine FPR coloring (retail f1, ours f3) remains. */
void rnd_bend_vector(PfxVec3* vector, float angle, float angle_spread) {
    PfxVec3 random_vector;
    PfxVec3 perpendicular;
    float sine;
    float cosine;
    float length;

    gxMathCosSin(&cosine, &sine,
        angle + rnd_between(-angle_spread, angle_spread));
    random_vector.x = rnd_between(-1.0f, 1.0f);
    random_vector.y = rnd_between(-1.0f, 1.0f);
    random_vector.z = rnd_between(-1.0f, 1.0f);
    length = rnd_length(vector);
    sine *= length;
    rnd_cross(&perpendicular, &random_vector, vector);
    rnd_normalize(&perpendicular, &perpendicular);
    perpendicular.x *= sine;
    perpendicular.y *= sine;
    perpendicular.z *= sine;
    vector->x = perpendicular.x + vector->x * cosine;
    vector->y = perpendicular.y + vector->y * cosine;
    vector->z = perpendicular.z + vector->z * cosine;
}
