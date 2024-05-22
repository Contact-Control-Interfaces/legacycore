//
// Created by john_contactci on 5/21/2024.
//

#pragma once

#include <ccic/lib_defs.h>

#include <optional>
#include <tuple>
#include <variant>

namespace contactci::haptic_state {
    class CCI_API_CLASS(VibrationEffect) {
    public:
        VibrationEffect(uint8_t effect, uint8_t modifier);

        uint8_t get_effect() const;
        uint8_t get_modifier() const;

    private:
        uint8_t effect;
        uint8_t modifier;
    };

    using ForceFeedbackState = std::variant<float, float>;
    enum class ForceFeedbackVariant : std::size_t {
        Amplitude = 0,
        Position = 1
    };

    using VibrationState = std::variant<float, VibrationEffect>;
    enum class VibrationVariant : std::size_t {
        Amplitude = 0,
        Effect = 1
    };

    using DigitState = std::tuple<
        std::optional<ForceFeedbackState>,
        std::optional<VibrationState>
    >;
    enum class HapticDimension : std::size_t {
        ForceFeedback = 0,
        Vibration = 1
    };

    using HandState = std::tuple<
        std::optional<DigitState>,
        std::optional<DigitState>,
        std::optional<DigitState>,
        std::optional<DigitState>,
        std::optional<DigitState>
    >;

    enum class Digit : std::size_t {
        Thumb = 0,
        Index = 1,
        Middle = 2,
        Ring = 3,
        Little = 4
    };

    enum class Hand : std::size_t {
        Left = 0,
        Right = 1
    };

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

    template <Digit digit>
    std::optional<DigitState> &get_digit(HandState &state) {
        return std::get<static_cast<::size_t>(digit)>(state);
    }

    template <Digit digit>
    const std::optional<DigitState> &get_digit(const HandState &state) {
        return std::get<static_cast<::size_t>(digit)>(state);
    }

    template <Hand hand>
    HandState &get_hand(std::tuple<HandState, HandState> &state) {
        return std::get<static_cast<::size_t>(hand)>(state);
    }

    template <Hand hand>
    const HandState &get_hand(const std::tuple<HandState, HandState> &state) {
        return std::get<static_cast<::size_t>(hand)>(state);
    }

    class CCI_API_CLASS(HapticStatePatch) {
        public:
            HapticStatePatch() = default;
            virtual ~HapticStatePatch() = default;

            template<Hand hand, Digit digit>
            HapticStatePatch &with_forcefeedback_amplitude(float amplitude);

            template<Hand hand, Digit digit>
            HapticStatePatch &with_forcefeedback_position(float position);

            template<Hand hand, Digit... digit>
            HapticStatePatch &with_vibration_amplitude(float amplitude);

            template<Hand hand, Digit digit>
            HapticStatePatch &with_vibration_effect(const VibrationEffect &effect);

            template<Hand hand, Digit digit>
            std::optional<float> forcefeedback_amplitude() const;

            template<Hand hand, Digit digit>
            std::optional<float> forcefeedback_position() const;

            template<Hand hand, Digit digit>
            std::optional<float> vibration_amplitude() const;

            template<Hand hand, Digit digit>
            std::optional<VibrationEffect> vibration_effect() const;

        private:
            template<Hand hand, Digit digit>
            HapticStatePatch &with_forcefeedback(const ForceFeedbackState &ffState);

            template<Hand hand, Digit... digit>
            HapticStatePatch &with_vibration(const VibrationState &vibrationState);

            template<Hand hand, Digit digit>
            const std::optional<ForceFeedbackState> &forcefeedback() const;

            template<Hand hand, Digit digit>
            const std::optional<VibrationState> &vibration() const;

            std::tuple<HandState, HandState> handStates;
    };
}