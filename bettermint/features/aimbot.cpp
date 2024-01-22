#include "features/aimbot.h"
#include <cmath>
#include <cstdlib>

static float rand_range_f(float f_min, float f_max) {
    float scale = rand() / (float)RAND_MAX;
    return f_min + scale * (f_max - f_min);
}

static constexpr float DEAD_ZONE_THRESHOLD = 1.0f;

template <typename T>
static inline T distance(const Vector2<T> &v1, const Vector2<T> &v2) {
    return std::sqrt(std::pow(v1.x - v2.x, 2) + std::pow(v1.y - v2.y, 2));
}

static inline Vector2<float> stableMousePosition() {
    Vector2<float> currentMousePos(.0f, .0f);
    uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
    if (!osu_manager) return currentMousePos;
    uintptr_t osu_ruleset_ptr = *(uintptr_t *)(osu_manager + OSU_MANAGER_RULESET_PTR_OFFSET);
    if (!osu_ruleset_ptr) return currentMousePos;
    currentMousePos.x = *(float *)(osu_ruleset_ptr + OSU_RULESET_MOUSE_X_OFFSET);
    currentMousePos.y = *(float *)(osu_ruleset_ptr + OSU_RULESET_MOUSE_Y_OFFSET);

    return currentMousePos;
}

static inline Vector2<float> randomizePosition(const Vector2<float> &position, float variation) {
    return Vector2<float>(
        position.x + rand_range_f(-variation, variation),
        position.y + rand_range_f(-variation, variation)
    );
}

static inline Vector2<float> moveTowards(const Vector2<float> &current, const Vector2<float> &target, float speed) {
    // Move current position towards the target with a specified speed
    float delta_x = target.x - current.x;
    float delta_y = target.y - current.y;
    float distance_to_target = std::sqrt(delta_x * delta_x + delta_y * delta_y);

    if (distance_to_target <= speed) {
        return target;
    } else {
        float scale = speed / distance_to_target;
        return Vector2<float>(current.x + delta_x * scale, current.y + delta_y * scale);
    }
}

void update_aimbot(Circle &circle, const int32_t audio_time) {
    if (!cfg_aimbot_lock)
        return;

    float t = cfg_fraction_modifier * ImGui::GetIO().DeltaTime;
    Vector2<float> cursor_pos = stableMousePosition();

    if (circle.type == HitObjectType::Circle || circle.type == HitObjectType::Slider) {
        Vector2<float> target = playfield_to_screen(circle.position);
        cursor_pos = moveTowards(cursor_pos, target, 500.0f * t);
    } else if (circle.type == HitObjectType::Spinner && audio_time >= circle.start_time) {
        auto &center = circle.position;
        constexpr float radius = 60.0f;
        constexpr float PI = 3.14159f;
        static float angle = .0f;
        Vector2<float> next_point_on_circle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));

        Vector2<float> target = playfield_to_screen(next_point_on_circle);
        cursor_pos = moveTowards(cursor_pos, target, 500.0f * t);

        float spin_variation = 0.1f;
        angle += cfg_spins_per_minute / (3 * PI) * ImGui::GetIO().DeltaTime + rand_range_f(-spin_variation, spin_variation);
    } else if (circle.type == HitObjectType::Slider && audio_time >= circle.start_time) {
        // Assuming 'slider_end_position' is a member of 'Circle'
        Vector2<float> slider_end = playfield_to_screen(circle.slider_end_position);
        cursor_pos = moveTowards(cursor_pos, slider_end, 500.0f * t);
    }

    move_mouse_to(cursor_pos.x, cursor_pos.y);
}
