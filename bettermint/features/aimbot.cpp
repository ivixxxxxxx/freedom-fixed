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

static inline Vector2<float> smoothCursorMovement(const Vector2<float> &current, const Vector2<float> &target, float t) {
    // Smoothly interpolate between current and target positions
    return Vector2<float>(
        current.x + (target.x - current.x) * t,
        current.y + (target.y - current.y) * t
    );
}

void update_aimbot(Circle &circle, const int32_t audio_time) {
    if (!cfg_aimbot_lock)
        return;

    float t = cfg_fraction_modifier * ImGui::GetIO().DeltaTime;
    Vector2<float> cursor_pos = stableMousePosition();

    if (circle.type == HitObjectType::Circle) {
        Vector2<float> target = playfield_to_screen(circle.position);
        cursor_pos = smoothCursorMovement(cursor_pos, target, t);
    } else if (circle.type == HitObjectType::Slider) {
        // ... (same as before)

        Vector2<float> slider_ball(slider_ball_x, slider_ball_y);
        slider_ball.x += rand_range_f(-slider_variation, slider_variation);
        slider_ball.y += rand_range_f(-slider_variation, slider_variation);

        Vector2<float> target = playfield_to_screen(slider_ball);
        cursor_pos = smoothCursorMovement(cursor_pos, target, t);
    } else if (circle.type == HitObjectType::Spinner && audio_time >= circle.start_time) {
        // ... (same as before)

        Vector2<float> next_point_on_circle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
        next_point_on_circle.x += rand_range_f(-spinner_variation, spinner_variation);
        next_point_on_circle.y += rand_range_f(-spinner_variation, spinner_variation);

        Vector2<float> target = playfield_to_screen(next_point_on_circle);
        cursor_pos = smoothCursorMovement(cursor_pos, target, t);
    }

    move_mouse_to(cursor_pos.x, cursor_pos.y);
}
