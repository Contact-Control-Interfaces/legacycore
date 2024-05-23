//
// Created by john_contactci on 5/21/2024.
//

#include "cci/haptic_state.h"
#include <ccic/haptic_state.h>

using namespace contactci;
using namespace contactci::haptic_state;

enum class HapticDimension : std::size_t {
    ForceFeedback = 0,
    Vibration = 1
};

enum class VibrationVariant : std::size_t {
    Amplitude = 0,
    Effect = 1
};

enum class ForceFeedbackVariant : std::size_t {
    Amplitude = 0,
    Position = 1
};

VibrationEffect::VibrationEffect(uint8_t effect, uint8_t modifier)
    : effect(effect), modifier(modifier) {}

uint8_t VibrationEffect::get_effect() const {
    return effect;
}

uint8_t VibrationEffect::get_modifier() const {
    return modifier;
}

HandIndexer::HandIndexer(const std::set<Hand> &hands, const std::set<Digit> &digits)
    : _hands(hands), _digits(digits) {}

const std::set<Hand> &HandIndexer::hands() const {
    return _hands;
}

const std::set<Digit> &HandIndexer::digits() const {
    return _digits;
}

template <typename T, ForceFeedbackVariant V>
const T *get_variant_if(const ForceFeedbackState &state) {
    return std::get_if<static_cast<::size_t>(V)>(&state);
}

template <typename T, ForceFeedbackVariant V>
T *get_variant_if(ForceFeedbackState &state) {
    return std::get_if<static_cast<::size_t>(V)>(&state);
}

template <typename T, VibrationVariant V>
const T *get_variant_if(const VibrationState &state) {
    return std::get_if<static_cast<::size_t>(V)>(&state);
}

template <typename T, VibrationVariant V>
T *get_variant_if(VibrationState &state) {
    return std::get_if<static_cast<::size_t>(V)>(&state);
}

template <typename T, HapticDimension dim>
std::optional<T> &get_dimension(DigitState &state) {
    return std::get<static_cast<::size_t>(dim)>(state);
}

template <typename T, HapticDimension dim>
const std::optional<T> &get_dimension(const DigitState &state) {
    return std::get<static_cast<::size_t>(dim)>(state);
}

std::optional<DigitState> &get_digit(HandState &state, Digit digit) {
    return state[static_cast<std::size_t>(digit)];
}

const std::optional<DigitState> &get_digit(const HandState &state, Digit digit) {
    return state[static_cast<std::size_t>(digit)];
}

HandState &get_hand(std::array<HandState, 2> &state, Hand hand) {
    return state[static_cast<std::size_t>(hand)];
}

const HandState &get_hand(const std::array<HandState, 2> &state, Hand hand) {
    return state[static_cast<std::size_t>(hand)];
}

contactci::haptic_state::ForceFeedbackState ff_amplitude(float amplitude) {
    return contactci::haptic_state::ForceFeedbackState(
        std::in_place_index<static_cast<std::size_t>(ForceFeedbackVariant::Amplitude)>, amplitude
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

std::optional<float> HapticStatePatch::forcefeedback_amplitude(Hand hand, Digit digit) const {
    std::optional<ForceFeedbackState> ffState = forcefeedback(hand, digit);

    if (!ffState.has_value())
        return std::nullopt;

    return ff_amplitude(*ffState);
}

std::optional<float> HapticStatePatch::forcefeedback_position(Hand hand, Digit digit) const {
    std::optional<ForceFeedbackState> ffState = forcefeedback(hand, digit);

    if (!ffState.has_value())
        return std::nullopt;

    return ff_position(*ffState);
}

std::optional<float> HapticStatePatch::vibration_amplitude(Hand hand, Digit digit) const {
    std::optional<VibrationState> vibState = vibration(hand, digit);

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_amplitude(*vibState);
}

std::optional<VibrationEffect> HapticStatePatch::vibration_effect(Hand hand, Digit digit) const {
    std::optional<VibrationState> vibState = vibration(hand, digit);

    if (!vibState.has_value())
        return std::nullopt;

    return ::vibration_effect(*vibState);
}

HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude(const HandIndexer &indexer, float amplitude) {
    return with_forcefeedback(indexer, ff_amplitude(amplitude));
}

HapticStatePatch &HapticStatePatch::with_forcefeedback_position(const HandIndexer &indexer, float position) {
    return with_forcefeedback(indexer, ff_position(position));
}

HapticStatePatch &HapticStatePatch::with_vibration_amplitude(const HandIndexer &indexer, float amplitude) {
    return with_vibration(indexer, ::vibration_amplitude(amplitude));
}

HapticStatePatch &HapticStatePatch::with_vibration_effect(const HandIndexer &indexer, const VibrationEffect &effect) {
    return with_vibration(indexer, ::vibration_effect(effect));
}

HapticStatePatch &HapticStatePatch::with_forcefeedback(const HandIndexer &indexer, const ForceFeedbackState &ffState) {
    for (Hand hand : indexer.hands()) {
        for (Digit digit : indexer.digits()) {
            std::optional<DigitState> &digitState = get_digit(get_hand(handStates, hand), digit);

            if (!digitState.has_value())
                digitState = DigitState();

            get_dimension<ForceFeedbackState, HapticDimension::ForceFeedback>(*digitState) = ffState;
        }
    }

    return *this;
}

HapticStatePatch &HapticStatePatch::with_vibration(const HandIndexer &indexer, const VibrationState &vibrationState) {
    for (Hand hand : indexer.hands()) {
        for (Digit digit : indexer.digits()) {
            std::optional<DigitState> &digitState = get_digit(get_hand(handStates, hand), digit);

            if (!digitState.has_value())
                digitState = DigitState();

            get_dimension<VibrationState, HapticDimension::Vibration>(*digitState) = vibrationState;
        }
    }

    return *this;
}

std::optional<ForceFeedbackState> HapticStatePatch::forcefeedback(Hand hand, Digit digit) const {
    const std::optional<DigitState> &state = get_digit(get_hand(handStates, hand), digit);

    if (!state.has_value())
        return std::nullopt;

    return get_dimension<ForceFeedbackState, HapticDimension::ForceFeedback>(state.value());
}

std::optional<VibrationState> HapticStatePatch::vibration(Hand hand, Digit digit) const {
    const std::optional<DigitState> &state = get_digit(get_hand(handStates, hand), digit);

    if (!state.has_value())
        return std::nullopt;

    return get_dimension<VibrationState, HapticDimension::Vibration>(state.value());
}

//// Explicit template instantiation for the template functions in HapticStatePatch.
//// This allows us to move the definitions of these template functions out of the header, keeping them private
//#define X(H, D) \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_amplitude<H, D>(float amplitude);          \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback_position<H, D>(float position);            \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_amplitude<H, D>(float amplitude);              \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration_effect<H, D>(const VibrationEffect &effect);   \
//    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_amplitude<H, D>() const;                     \
//    template __declspec(dllexport) std::optional<float> HapticStatePatch::forcefeedback_position<H, D>() const;                      \
//    template __declspec(dllexport) std::optional<float> HapticStatePatch::vibration_amplitude<H, D>() const;                         \
//    template __declspec(dllexport) std::optional<VibrationEffect> HapticStatePatch::vibration_effect<H, D>() const;                  \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_forcefeedback<H, D>(const ForceFeedbackState &ffState);  \
//    template __declspec(dllexport) HapticStatePatch &HapticStatePatch::with_vibration<H, D>(const VibrationState &vibrationState);   \
//    template __declspec(dllexport) std::optional<ForceFeedbackState> HapticStatePatch::forcefeedback<H, D>() const;           \
//    template __declspec(dllexport) std::optional<VibrationState> HapticStatePatch::vibration<H, D>() const;
//
//#define Y(H)            \
//    X(H, Digit::Thumb)     \
//    X(H, Digit::Index)     \
//    X(H, Digit::Middle)    \
//    X(H, Digit::Ring)      \
//    X(H, Digit::Little)
//
//Y(Hand::Left)
//Y(Hand::Right)
//
//#undef Y
//#undef X