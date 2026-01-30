#include <Empire/types.h>

#include <math.h>

static inline emp_vec2_t emp_vec2_add(emp_vec2_t a, emp_vec2_t b)
{
    return (emp_vec2_t){ a.x + b.x, a.y + b.y };
}

static inline emp_vec2_t emp_vec2_sub(emp_vec2_t a, emp_vec2_t b)
{
    return (emp_vec2_t){ a.x - b.x, a.y - b.y };
}

static inline emp_vec2_t emp_vec2_mul(emp_vec2_t v, float s)
{
    return (emp_vec2_t){ v.x * s, v.y * s };
}

static inline emp_vec2_t emp_vec2_mulv(emp_vec2_t a, emp_vec2_t b)
{
    return (emp_vec2_t){ a.x * b.x, a.y * b.y };
}

static inline emp_vec2_t emp_vec2_div(emp_vec2_t v, float s)
{
    return (emp_vec2_t){ v.x / s, v.y / s };
}

static inline float emp_vec2_dot(emp_vec2_t a, emp_vec2_t b)
{
    return (a.x * b.x) + (a.y * b.y);
}

static inline float emp_vec2_length_sq(emp_vec2_t v)
{
    return (v.x * v.x) + (v.y * v.y);
}

static inline float emp_vec2_length(emp_vec2_t v)
{
    return sqrtf(emp_vec2_length_sq(v));
}

static inline emp_vec2_t emp_vec2_normalize(emp_vec2_t v)
{
    float len = emp_vec2_length(v);
    if (len > 0.0f) {
        return emp_vec2_div(v, len);
    }
    return (emp_vec2_t){ 0.0f, 0.0f };
}

static inline float emp_vec2_dist_sq(emp_vec2_t a, emp_vec2_t b)
{
    return emp_vec2_length_sq(emp_vec2_sub(b, a));
}

static inline float emp_vec2_dist(emp_vec2_t a, emp_vec2_t b)
{
    return sqrtf(emp_vec2_dist_sq(a, b));
}

static inline emp_vec2_t emp_vec2_lerp(emp_vec2_t a, emp_vec2_t b, float t)
{
    return (emp_vec2_t){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

static inline float emp_deg_to_rad(float deg)
{
    return deg * (3.14159265358979323846f / 180.0f);
}

static inline emp_vec2_t emp_vec2_rotate(emp_vec2_t v, float deg)
{
    float rad = emp_deg_to_rad(deg);
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    return (emp_vec2_t) {
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
}