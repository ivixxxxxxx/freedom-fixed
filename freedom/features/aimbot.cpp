#include "features/aimbot.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline Vector2<float> mouse_position()
{
    Vector2<float> mouse_pos(.0f, .0f);
    uintptr_t osu_manager = *(uintptr_t*)(osu_manager_ptr);
    if (!osu_manager) return mouse_pos;
    uintptr_t osu_ruleset_ptr = *(uintptr_t*)(osu_manager + OSU_MANAGER_RULESET_PTR_OFFSET);
    if (!osu_ruleset_ptr) return mouse_pos;
    mouse_pos.x = *(float*)(osu_ruleset_ptr + OSU_RULESET_MOUSE_X_OFFSET);
    mouse_pos.y = *(float*)(osu_ruleset_ptr + OSU_RULESET_MOUSE_Y_OFFSET);
    return mouse_pos;
}

static inline Vector2<float> lerp(const Vector2<float> &a, const Vector2<float> &b, float t)
{
    return Vector2<float>(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y)
    );
}

static inline void smooth_move_mouse_to(const Vector2<float> &target, const Vector2<float> &cursor_pos)
{
    constexpr float smoothingFactor = 0.1f;
    Vector2<float> smoothed_position = lerp(cursor_pos, target, smoothingFactor);
    move_mouse_to(smoothed_position.x, smoothed_position.y);
}

static inline void smooth_move_mouse_to_target(const Vector2<float> &target, const Vector2<float> &cursor_pos, float t)
{
    constexpr float circle_radius = 100.0f; // Adjust as needed
    float circle_speed = cfg_fraction_modifier; // Adjust based on the game dynamics
    static float angle = 0.0f;

    // Calculate the center of the circular motion (right side)
    Vector2<float> circle_center(cursor_pos.x + circle_radius, cursor_pos.y);

    // Check if the target is close enough
    Vector2<float> target_on_screen = playfield_to_screen(target);
    Vector2<float> direction = target_on_screen - cursor_pos;
    constexpr float target_radius = 10.0f; // Adjust as needed

    if (direction.length() > target_radius) {
        // Target found, move towards it
        // Calculate the angle between the direction and the x-axis
        float target_angle = atan2(direction.y, direction.x);

        // If the target is on the left side of the circle center, speed up
        if (target_on_screen.x < circle_center.x) {
            circle_speed *= 2.0f; // Double the speed
        }

        // Move the mouse cursor towards the target along a circular path
        Vector2<float> next_position(
            circle_center.x + circle_radius * cos(target_angle),
            circle_center.y + circle_radius * sin(target_angle)
        );

        // Move the cursor towards the next position on the circular path
        smooth_move_mouse_to(next_position, cursor_pos);
    } else {
        // Move the mouse cursor in a circular motion if no target is found
        Vector2<float> next_position(
            cursor_pos.x + circle_radius * cos(angle),
            cursor_pos.y + circle_radius * sin(angle)
        );

        // Move the cursor towards the next position on the circle
        smooth_move_mouse_to(next_position, cursor_pos);

        // Update the angle for the next frame
        angle += circle_speed * t;
        if (angle >= 2 * M_PI)
            angle -= 2 * M_PI;
    }
}

void update_aimbot(Circle &circle, const int32_t audio_time)
{
    if (!cfg_aimbot_lock)
        return;

    constexpr float fixedDeltaTime = 1.0f / 60.0f;
    float t = cfg_fraction_modifier * fixedDeltaTime;
    Vector2<float> cursor_pos = mouse_position();

    if (circle.type == HitObjectType::Circle)
    {
        smooth_move_mouse_to_target(circle.position, cursor_pos, t);
    }
    else if (circle.type == HitObjectType::Slider)
    {
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
        Vector2<float> slider_ball(slider_ball_x, slider_ball_y);
        smooth_move_mouse_to_target(slider_ball, cursor_pos, t);
    }
    else if (circle.type == HitObjectType::Spinner && audio_time >= circle.start_time)
    {
        auto& center = circle.position;
        constexpr float radius = 60.0f;
        constexpr float PI = M_PI;
        static float angle = .0f;
        Vector2 next_point_on_circle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
        smooth_move_mouse_to_target(next_point_on_circle, cursor_pos, t);
        float three_pi = 3 * PI;
        if (cfg_timewarp_enabled)
        {
            auto timewarp_playback_rate = cfg_timewarp_playback_rate / 100.0;
            if (timewarp_playback_rate > 1.0)
                three_pi /= timewarp_playback_rate;
        }
        angle > 2 * PI ? angle = 0 : angle += cfg_spins_per_minute / three_pi * ImGui::GetIO().DeltaTime;
    }
}
