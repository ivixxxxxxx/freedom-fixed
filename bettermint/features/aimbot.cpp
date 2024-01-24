#include "Vector.h"
#include "hitobject.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>

using u32 = std::uint32_t;

// Fixed rand_range_f definition
float rand_range_f(float f_min, float f_max);

// Fixed smoothStep definition
inline float smoothStep(float edge0, float edge1, float x);

// Fixed easeInOutQuad definition
inline float easeInOutQuad(float t);

// Fixed lerpWithEase definition
inline float lerpWithEase(float a, float b, float t);

// Fixed distance template definition
template <typename T>
T distance(const Vector2<T>& v1, const Vector2<T>& v2);

#ifndef MY_PI
constexpr float MY_PI = 3.14159f;
#endif

constexpr float DEAD_ZONE_THRESHOLD = 0.5f; // Adjust as needed

namespace aimbot {
    void update_aimbot(Circle& circle, const int32_t audio_time);
    float rand_range_f(float f_min, float f_max);
    inline float smoothStep(float edge0, float edge1, float x);
    inline float easeInOutQuad(float t);
    inline float lerpWithEase(float a, float b, float t);

    template <typename T>
    T distance(const Vector2<T>& v1, const Vector2<T>& v2);

    template <>
    float distance(const Vector2<float>& v1, const Vector2<float>& v2) {
        return std::sqrt(std::pow(v1.x - v2.x, 2) + std::pow(v1.y - v2.y, 2));
    }

    inline Vector2<float> stableMousePosition();

    inline void move_mouse_to_target(const Vector2<float>& target, const Vector2<float>& cursor_pos, float t);

    void update_aimbot(Circle& circle, const int32_t audio_time);
}
        // Fixed rand_range_f definition
        float rand_range_f(float f_min, float f_max) {
            float scale = rand() / static_cast<float>(RAND_MAX);
            return f_min + scale * (f_max - f_min);
        }

        // Fixed smoothStep definition
        inline float smoothStep(float edge0, float edge1, float x) {
            float t = fmaxf(0.0f, fminf(1.0f, (x - edge0) / (edge1 - edge0)));
            return t * t * (3.0f - 2.0f * t);
        }

        // Fixed easeInOutQuad definition
        inline float easeInOutQuad(float t) {
            return t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }

        // Fixed lerpWithEase definition
        inline float lerpWithEase(float a, float b, float t) {
            t = smoothStep(0.0f, 1.0f, t);
            return a + t * (b - a);
        }

        // Fixed distance template definition
        template <typename T>
        T distance(const Vector2<T>& v1, const Vector2<T>& v2) {
            return std::sqrt(std::pow(v1.x - v2.x, 2) + std::pow(v1.y - v2.y, 2));
        }

        // Add explicit instantiation for float
        template <>
        float distance(const Vector2<float>& v1, const Vector2<float>& v2) {
            return std::sqrt(std::pow(v1.x - v2.x, 2) + std::pow(v1.y - v2.y, 2));
        }

    inline Vector2<float> stableMousePosition() {
        Vector2<float> currentMousePos(.0f, .0f);
        uintptr_t osu_manager = *(uintptr_t*)(osu_manager_ptr);
        if (!osu_manager) return currentMousePos;
        uintptr_t osu_ruleset_ptr = *(uintptr_t*)(osu_manager + OSU_MANAGER_RULESET_PTR_OFFSET);
        if (!osu_ruleset_ptr) return currentMousePos;
        currentMousePos.x = *(float*)(osu_ruleset_ptr + OSU_RULESET_MOUSE_X_OFFSET);
        currentMousePos.y = *(float*)(osu_ruleset_ptr + OSU_RULESET_MOUSE_Y_OFFSET);

        static Vector2<float> lastMousePos = currentMousePos;

if (aimbot::distance<float>(currentMousePos, lastMousePos) < DEAD_ZONE_THRESHOLD) {
            return lastMousePos;
        }

        lastMousePos = currentMousePos;
        return currentMousePos;
    }

inline void move_mouse_to_target(const Vector2<float>& target, const Vector2<float>& cursor_pos, float t) {
    Vector2 target_on_screen = playfield_to_screen(target);

    float movement_speed = 50.0f; // Adjust as needed

    // Calculate the direction vector
    Vector2 direction = normalize(target_on_screen - cursor_pos);

    // Calculate the new cursor position based on speed and time
    Vector2 new_cursor_pos = cursor_pos + direction * movement_speed * t;

    // Update the cursor position
    move_mouse_to(new_cursor_pos.x, new_cursor_pos.y);
}

    void update_aimbot(Circle& circle, const int32_t audio_time) {
        if (!cfg_aimbot_lock)
            return;

        float t = cfg_fraction_modifier * ImGui::GetIO().DeltaTime;
        Vector2<float> cursor_pos = stableMousePosition();

        if (circle.type == HitObjectType::Circle) {
            move_mouse_to_target(circle.position, cursor_pos, t);
        }
        else if (circle.type == HitObjectType::Slider) {
            uintptr_t osu_manager = *(uintptr_t*)(osu_manager_ptr);
            if (!osu_manager) return;
            uintptr_t hit_manager_ptr = *(uintptr_t*)(osu_manager + OSU_MANAGER_HIT_MANAGER_OFFSET);
            if (!hit_manager_ptr) return;
            uintptr_t hit_objects_list_ptr = *(uintptr_t*)(hit_manager_ptr + OSU_HIT_MANAGER_HIT_OBJECTS_LIST_OFFSET);
            uintptr_t hit_objects_list_items_ptr = *(uintptr_t*)(hit_objects_list_ptr + 0x4);
            uintptr_t hit_object_ptr = *(uintptr_t*)(hit_objects_list_items_ptr + 0x8 + 0x4 * current_beatmap.hit_object_idx);
            uintptr_t animation_ptr = *(uintptr_t*)(hit_object_ptr + OSU_HIT_OBJECT_ANIMATION_OFFSET);
            float slider_ball_x = *(float*)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_X_OFFSET);
            float slider_ball_y = *(float*)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_Y_OFFSET);
            Vector2 slider_ball(slider_ball_x, slider_ball_y);

            float slider_variation = 0.0f;
            slider_ball.x += rand_range_f(-slider_variation, slider_variation);
            slider_ball.y += rand_range_f(-slider_variation, slider_variation);

            move_mouse_to_target(slider_ball, cursor_pos, t);
        }
        else if (circle.type == HitObjectType::Spinner && audio_time >= circle.start_time) {
            auto& center = circle.position;
            constexpr float radius = 60.0f;
            constexpr float PI = 3.14159f;
            static float angle = .0f;
            Vector2 next_point_on_circle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));

            float spinner_variation = 20.0f;
            next_point_on_circle.x += rand_range_f(-spinner_variation, spinner_variation);
            next_point_on_circle.y += rand_range_f(-spinner_variation, spinner_variation);

            move_mouse_to_target(next_point_on_circle, cursor_pos, t);

            float spin_variation = 0.1f;
            angle += cfg_spins_per_minute / (3 * PI) * ImGui::GetIO().DeltaTime + rand_range_f(-spin_variation, spin_variation);
        }
    };
