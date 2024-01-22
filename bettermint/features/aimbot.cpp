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

static inline Vector2<float> mouse_position() {
    Vector2<float> mouse_pos(.0f, .0f);
    uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
    if (!osu_manager) return mouse_pos;
    uintptr_t osu_ruleset_ptr = *(uintptr_t *)(osu_manager + OSU_MANAGER_RULESET_PTR_OFFSET);
    if (!osu_ruleset_ptr) return mouse_pos;
    mouse_pos.x = *(float *)(osu_ruleset_ptr + OSU_RULESET_MOUSE_X_OFFSET);
    mouse_pos.y = *(float *)(osu_ruleset_ptr + OSU_RULESET_MOUSE_Y_OFFSET);
    return mouse_pos;
}

static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static inline void move_mouse_to_target(const Vector2<float> &target, const Vector2<float> &cursor_pos, float t) {
    Vector2 target_on_screen = playfield_to_screen(target);
    Vector2 predicted_position(lerp(cursor_pos.x, target_on_screen.x, t), lerp(cursor_pos.y, target_on_screen.y, t));
    move_mouse_to(predicted_position.x, predicted_position.y);
}

void update_aimbot(Circle &circle, const int32_t audio_time) {
    if (!cfg_aimbot_lock)
        return;

    float t = cfg_fraction_modifier * ImGui::GetIO().DeltaTime;
    Vector2 cursor_pos = mouse_position();

    if (circle.type == HitObjectType::Circle || circle.type == HitObjectType::Slider) {
        Vector2<float> target = playfield_to_screen(circle.position);

        // Check if the hit object is a slider
        if (circle.type == HitObjectType::Slider) {
            uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
            if (!osu_manager) return;
            uintptr_t hit_manager_ptr = *(uintptr_t *)(osu_manager + OSU_MANAGER_HIT_MANAGER_OFFSET);
            if (!hit_manager_ptr) return;
            uintptr_t hit_objects_list_ptr = *(uintptr_t *)(hit_manager_ptr + OSU_HIT_MANAGER_HIT_OBJECTS_LIST_OFFSET);
            uintptr_t hit_objects_list_items_ptr = *(uintptr_t *)(hit_objects_list_ptr + 0x4);
            uintptr_t hit_object_ptr = *(uintptr_t *)(hit_objects_list_items_ptr + 0x8 + 0x4 * current_beatmap.hit_object_idx);
            uintptr_t animation_ptr = *(uintptr_t *)(hit_object_ptr + OSU_HIT_OBJECT_ANIMATION_OFFSET);
            float slider_ball_x = *(float *)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_X_OFFSET);
            float slider_ball_y = *(float *)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_Y_OFFSET);
            target = Vector2<float>(slider_ball_x, slider_ball_y);
        }

        // Smoothly move towards the target based on the logic for circles and sliders
        cursor_pos = moveTowards(cursor_pos, target, 500.0f * t);
    } else if (circle.type == HitObjectType::Spinner && audio_time >= circle.start_time) {
        // Logic for spinners
        auto &center = circle.position;
        constexpr float radius = 60.0f;
        constexpr float PI = 3.14159f;
        static float angle = .0f;
        Vector2<float> next_point_on_circle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));

        // Smoothly move towards the next point on the spinner circle
        cursor_pos = moveTowards(cursor_pos, playfield_to_screen(next_point_on_circle), 500.0f * t);

        float three_pi = 3 * PI;
        if (cfg_timewarp_enabled) {
            auto timewarp_playback_rate = cfg_timewarp_playback_rate / 100.0;
            if (timewarp_playback_rate > 1.0)
                three_pi /= timewarp_playback_rate;
        }
        angle > 2 * PI ? angle = 0 : angle += cfg_spins_per_minute / three_pi * ImGui::GetIO().DeltaTime;
    }

    move_mouse_to(cursor_pos.x, cursor_pos.y);
}
