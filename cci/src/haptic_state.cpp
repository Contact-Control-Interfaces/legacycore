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
    const auto *amplitude = get_variant_if<float, ForceFeedbackVariant::Amplitude>(state);
    return amplitude == nullptr ? std::nullopt : std::make_optional(*amplitude);
}

std::optional<float> ff_position(const ForceFeedbackState &state) {
    const auto *position = get_variant_if<float, ForceFeedbackVariant::Position>(state);
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
    const auto *amplitude = get_variant_if<float, VibrationVariant::Amplitude>(state);
    return amplitude == nullptr ? std::nullopt : std::make_optional(*amplitude);
}

std::optional<VibrationEffect> vibration_effect(const VibrationState &state) {
    const auto *effect = get_variant_if<VibrationEffect, VibrationVariant::Effect>(state);
    return effect == nullptr ? std::nullopt : std::make_optional(*effect);
}

template<Hand hand, Digit digit>
std::optional<float> HapticStatePatch::forcefeedback_amplitude() const {
    std::optional<ForceFeedbackState> ffState = forcefeedback<hand, digit>();

    if (!ffState.has_value())
        return std::nullopt;

    return ff_amplitude(*ffState);
}

template<Hand hand, Digit digit>
std::optional<float> HapticStatePatch::forcefeedback_position() const {
    std::optional<ForceFeedbackState> ffState = forcefeedback<hand, digit>();

    if (!ffState.has_value())
        return std::nullopt;

    return ff_position(*ffState);
}

template<Hand hand, Digit digit>
std::optional<float> HapticStatePatch::vibration_amplitude() const {
    std::optional<VibrationState> vibState = vibration<hand, digit>();

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_amplitude(*vibState);
}

template<Hand hand, Digit digit>
std::optional<VibrationEffect> HapticStatePatch::vibration_effect() const {
    std::optional<VibrationState> vibState = vibration<hand, digit>();

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_effect(*vibState);
}

template<Hand hand, Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude(float amplitude) {
    return with_forcefeedback<hand, digit>(ff_amplitude(amplitude));
}

template<Hand hand, Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback_position(float position) {
    return with_forcefeedback<hand, digit>(ff_position(position));
}

template<Hand hand, Digit... digit>
HapticStatePatch &HapticStatePatch::with_vibration_amplitude(float amplitude) {
    return with_vibration<hand, digit...>(::vibration_amplitude(amplitude));
}

template<Hand hand, Digit digit>
HapticStatePatch &HapticStatePatch::with_vibration_effect(const VibrationEffect &effect) {
    return with_vibration<hand, digit>(::vibration_effect(effect));
}

template<Hand hand, Digit digit>
HapticStatePatch &HapticStatePatch::with_forcefeedback(const ForceFeedbackState &ffState) {
    std::optional<DigitState> &digitState = get_digit<digit>(get_hand<hand>(handStates));

    if (!digitState.has_value())
        digitState = DigitState();

    get_dimension<ForceFeedbackState, HapticDimension::ForceFeedback>(*digitState) = ffState;
    return *this;
}

template<Hand hand, Digit... digit>
HapticStatePatch &HapticStatePatch::with_vibration(const VibrationState &vibrationState) {
    std::optional<DigitState> &digitState = get_digit<digit...>(get_hand<hand>(handStates));

    if (!digitState.has_value())
        digitState = DigitState();

    get_dimension<VibrationState, HapticDimension::Vibration>(*digitState) = vibrationState;
    return *this;
}

template<Hand hand, Digit digit>
const std::optional<ForceFeedbackState> &HapticStatePatch::forcefeedback() const {
    return get_dimension<ForceFeedbackState, HapticDimension::ForceFeedback>(*get_digit<digit>(get_hand<hand>(handStates)));
}

template<Hand hand, Digit digit>
const std::optional<VibrationState> &HapticStatePatch::vibration() const {
    return get_dimension<VibrationState, HapticDimension::Vibration>(*get_digit<digit>(get_hand<hand>(handStates)));
}

// Explicit template instantiation for the template functions in HapticStatePatch.
// This allows us to move the definitions of these template functions out of the header, keeping them private
#define X(H, D) \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude<H, D>(float amplitude);          \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_position<H, D>(float position);            \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_amplitude<H, D>(float amplitude);              \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_effect<H, D>(const VibrationEffect &effect);   \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_amplitude<H, D>() const;                     \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_position<H, D>() const;                      \
    template __declspec(dllexport) std::optional<float> HapticStatePatch::vibration_amplitude<H, D>() const;                         \
    template __declspec(dllexport) std::optional<VibrationEffect> HapticStatePatch::vibration_effect<H, D>() const;                  \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback<H, D>(const ForceFeedbackState &ffState);  \
    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration<H, D>(const VibrationState &vibrationState);   \
    template __declspec(dllexport) const std::optional<ForceFeedbackState> &HapticStatePatch::forcefeedback<H, D>() const;           \
    template __declspec(dllexport) const std::optional<VibrationState> &HapticStatePatch::vibration<H, D>() const;
#define Y(H)            \
    X(H, Digit::Thumb)     \
    X(H, Digit::Index)     \
    X(H, Digit::Middle)    \
    X(H, Digit::Ring)      \
    X(H, Digit::Little)

Y(Hand::Left)
Y(Hand::Right)

#undef Y
#undef X