#include "features/aimbot.h"
#include <cmath>
#include <cstdlib>

// Constants
constexpr static float MOVEMENT_VARIATION = 1.5f;
constexpr static float SLIDER_VARIATION = 5.0f;
constexpr static float SPINNER_VARIATION = 10.0f;
constexpr static float SPIN_VARIATION = 0.1f;

// Function to calculate distance between two points
template <typename T>
static inline T calculateDistance(const Vector2<T> &v1, const Vector2<T> &v2) {
    return std::sqrt(std::pow(v1.x - v2.x, 2) + std::pow(v1.y - v2.y, 2));
}

// Function to move mouse to the target with variations
static inline void moveMouseToTargetWithVariation(const Vector2<float> &target, const Vector2<float> &cursorPos, float t, float variation) {
    Vector2 targetOnScreen = playfield_to_screen(target);

    targetOnScreen.x += rand_range_f(-variation, variation);
    targetOnScreen.y += rand_range_f(-variation, variation);

    Vector2 predictedPosition(lerpWithEase(cursorPos.x, targetOnScreen.x, t), lerpWithEase(cursorPos.y, targetOnScreen.y, t));
    move_mouse_to(predictedPosition.x, predictedPosition.y);
}

// Function to update aimbot for circles
void updateAimbotForCircle(Circle &circle, const Vector2<float> &cursorPos, float t) {
    moveMouseToTargetWithVariation(circle.position, cursorPos, t, MOVEMENT_VARIATION);
}

// Function to update aimbot for sliders
void updateAimbotForSlider(Circle &circle, const Vector2<float> &cursorPos, float t) {
    uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
    if (!osu_manager) return;
    uintptr_t hitManagerPtr = *(uintptr_t *)(osu_manager + OSU_MANAGER_HIT_MANAGER_OFFSET);
    if (!hitManagerPtr) return;
    uintptr_t hitObjectsListPtr = *(uintptr_t *)(hitManagerPtr + OSU_HIT_MANAGER_HIT_OBJECTS_LIST_OFFSET);
    uintptr_t hitObjectsListItemsPtr = *(uintptr_t *)(hitObjectsListPtr + 0x4);
    uintptr_t hitObjectPtr = *(uintptr_t *)(hitObjectsListItemsPtr + 0x8 + 0x4 * current_beatmap.hit_object_idx);
    uintptr_t animationPtr = *(uintptr_t *)(hitObjectPtr + OSU_HIT_OBJECT_ANIMATION_OFFSET);
    float sliderBallX = *(float *)(animationPtr + OSU_ANIMATION_SLIDER_BALL_X_OFFSET);
    float sliderBallY = *(float *)(animationPtr + OSU_ANIMATION_SLIDER_BALL_Y_OFFSET);
    Vector2 sliderBall(sliderBallX, sliderBallY);

    moveMouseToTargetWithVariation(sliderBall, cursorPos, t, SLIDER_VARIATION);
}

// Function to update aimbot for spinners
void updateAimbotForSpinner(Circle &circle, const Vector2<float> &cursorPos, float t, int32_t audioTime) {
    auto &center = circle.position;
    constexpr float radius = 60.0f;
    constexpr float PI = 3.14159f;
    static float angle = .0f;
    Vector2 nextPointOnCircle(center.x + radius * cosf(angle), center.y + radius * sinf(angle));

    moveMouseToTargetWithVariation(nextPointOnCircle, cursorPos, t, SPINNER_VARIATION);

    float spinVariation = 0.1f;
    angle += cfg_spins_per_minute / (3 * PI) * ImGui::GetIO().DeltaTime + rand_range_f(-spinVariation, spinVariation);
}

// Main update aimbot function
void updateAimbot(Circle &circle, const int32_t audioTime) {
    if (!cfg_aimbot_lock)
        return;

    float t = cfg_fraction_modifier * ImGui::GetIO().DeltaTime;
    Vector2<float> cursorPos = stableMousePosition();

    switch (circle.type) {
        case HitObjectType::Circle:
            updateAimbotForCircle(circle, cursorPos, t);
            break;

        case HitObjectType::Slider:
            updateAimbotForSlider(circle, cursorPos, t);
            break;

        case HitObjectType::Spinner:
            if (audioTime >= circle.start_time)
                updateAimbotForSpinner(circle, cursorPos, t, audioTime);
            break;

        // Add more cases if needed

        default:
            break;
    }
}
