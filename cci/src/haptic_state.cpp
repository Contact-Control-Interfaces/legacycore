//
// Created by john_contactci on 5/21/2024.
//

#include "cci/haptic_state.h"
#include <ccic/haptic_state.h>

using namespace contactci;
using namespace contactci::haptic_state;

VibrationEffect::VibrationEffect(uint8_t effect, uint8_t modifier)
    : effect(effect), modifier(modifier) {}

uint8_t VibrationEffect::get_effect() const {
    return effect;
}

uint8_t VibrationEffect::get_modifier() const {
    return modifier;
}

contactci::haptic_state::ForceFeedbackState ff_amplitude(float amplitude) {
    return contactci::haptic_state::ForceFeedbackState(
        std::in_place_index<static_cast<std::size_t>(contactci::haptic_state::ForceFeedbackVariant::Amplitude)>, amplitude
    );
}

ForceFeedbackState ff_position(float position) {
    return ForceFeedbackState(
        std::in_place_index<static_cast<std::size_t>(ForceFeedbackVariant::Position)>, position
    );
}

std::optional<float> ff_amplitude(const ForceFeedbackState &state) {
    const float *amplitude = std::get_if<static_cast<std::size_t>(ForceFeedbackVariant::Amplitude)>(&state);
    return amplitude == nullptr ? std::nullopt : std::make_optional(*amplitude);
}

std::optional<float> ff_position(const ForceFeedbackState &state) {
    const float *position = std::get_if<static_cast<std::size_t>(ForceFeedbackVariant::Position)>(&state);
    return position == nullptr ? std::nullopt : std::make_optional(*position);
}

VibrationState vibration_amplitude(float amplitude) {
    return VibrationState(
            std::in_place_index<static_cast<std::size_t>(VibrationVariant::Amplitude)>, amplitude
    );
}

VibrationState vibration_effect(const VibrationEffect &effect) {
    return VibrationState(
            std::in_place_index<static_cast<std::size_t>(VibrationVariant::Effect)>, effect
    );
}

std::optional<float> vibration_amplitude(const VibrationState &state) {
    const float *amplitude = std::get_if<static_cast<std::size_t>(VibrationVariant::Amplitude)>(&state);
    return amplitude == nullptr ? std::nullopt : std::make_optional(*amplitude);
}

std::optional<VibrationEffect> vibration_effect(const VibrationState &state) {
    const VibrationEffect *effect = std::get_if<static_cast<std::size_t>(VibrationVariant::Effect)>(&state);
    return effect == nullptr ? std::nullopt : std::make_optional(*effect);
}

template<Digit digit>
std::optional<float> HapticStatePatch::forcefeedback_amplitude() {
    std::optional<ForceFeedbackState> ffState = get_forcefeedback<digit>();

    if (!ffState.has_value())
        return std::nullopt;

    return ff_amplitude(*ffState);
}

template<Digit digit>
std::optional<float> HapticStatePatch::forcefeedback_position() {
    std::optional<ForceFeedbackState> ffState = get_forcefeedback<digit>();

    if (!ffState.has_value())
        return std::nullopt;

    return ff_position(*ffState);
}

template<Digit digit>
std::optional<float> HapticStatePatch::vibration_amplitude() {
    std::optional<VibrationState> vibState = get_vibration<digit>();

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_amplitude(*vibState);
}

template<Digit digit>
std::optional<VibrationEffect> HapticStatePatch::vibration_effect() {
    std::optional<VibrationState> vibState = get_vibration<digit>();

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_effect(*vibState);
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude(float amplitude) {
    return with_forcefeedback<digit>(ff_amplitude(amplitude));
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback_position(float position) {
    return with_forcefeedback<digit>(ff_position(position));
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_vibration_amplitude(float amplitude) {
    return with_vibration<digit>(::vibration_amplitude(amplitude));
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_vibration_effect(const VibrationEffect &effect) {
    return with_vibration<digit>(::vibration_effect(effect));
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback(const ForceFeedbackState &ffState) {
    std::optional<DigitState> &digitState = std::get<static_cast<std::size_t>(digit)>(digitStates);

    if (!digitState.has_value())
        digitState = DigitState();

    std::get<static_cast<std::size_t>(DigitStateMember::ForceFeedback)>(*digitState) = ffState;
    return *this;
}

template<Digit digit>
HapticStatePatch &HapticStatePatch::with_vibration(const VibrationState &vibrationState) {
    std::get<static_cast<std::size_t>(DigitStateMember::Vibration)>(
            *std::get<static_cast<std::size_t>(digit)>(digitStates)
    ) = vibrationState;
    return *this;
}

template<Digit digit>
const std::optional<DigitState> &HapticStatePatch::get_digit_state() const {
    return std::get<static_cast<std::size_t>(digit)>(digitStates);
}

template<Digit digit>
const std::optional<ForceFeedbackState> &HapticStatePatch::get_forcefeedback() const {
    return std::get<static_cast<std::size_t>(DigitStateMember::ForceFeedback)>(
            *std::get<static_cast<std::size_t>(digit)>(digitStates)
    );
}

template<Digit digit>
const std::optional<VibrationState> &HapticStatePatch::get_vibration() const {
    return std::get<static_cast<std::size_t>(DigitStateMember::Vibration)>(
            *std::get<static_cast<std::size_t>(digit)>(digitStates)
    );
}

// Explicit template instantiation for the template functions in HapticStatePatch.
// This allows us to move the definitions of these template functions out of the header, keeping them private
#define X(T) \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude<T>(float amplitude);          \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_position<T>(float position);            \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_amplitude<T>(float amplitude);              \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_effect<T>(const VibrationEffect &effect);   \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_amplitude<T>();                           \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_position<T>();                            \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::vibration_amplitude<T>();                               \
    template __declspec(dllexport) std::optional<VibrationEffect> HapticStatePatch::vibration_effect<T>();                        \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback<T>(const ForceFeedbackState &ffState);  \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration<T>(const VibrationState &vibrationState);   \
    template __declspec(dllexport) const std::optional<DigitState> &HapticStatePatch::get_digit_state<T>() const;                 \
    template __declspec(dllexport) const std::optional<ForceFeedbackState> &HapticStatePatch::get_forcefeedback<T>() const;       \
    template __declspec(dllexport) const std::optional<VibrationState> &HapticStatePatch::get_vibration<T>() const;

X(Digit::Thumb)
X(Digit::Index)
X(Digit::Middle)
X(Digit::Ring)
X(Digit::Little)

#undef X