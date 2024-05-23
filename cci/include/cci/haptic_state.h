//
// Created by john_contactci on 5/21/2024.
//

#pragma once

#include <ccic/lib_defs.h>

#include <optional>
#include <tuple>
#include <variant>
#include <set>
#include <array>

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
    using VibrationState = std::variant<float, VibrationEffect>;
    using DigitState = std::tuple<
        std::optional<ForceFeedbackState>,
        std::optional<VibrationState>
    >;
    using HandState = std::array<std::optional<DigitState>, 5>;

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

    class CCI_API_CLASS(HandIndexer) {
    public:
        HandIndexer(const std::set<Hand> &hands, const std::set<Digit> &digits);
        const std::set<Hand> &hands() const;
        const std::set<Digit> &digits() const;

    private:
        std::set<Hand> _hands;
        std::set<Digit> _digits;
    };

    inline HandIndexer for_digits(const std::set<Hand> &hands, const std::set<Digit> &digits) {
        return {hands, digits};
    }

    inline HandIndexer for_digits(const std::set<Hand> &hands, Digit digit) {
        return {hands, std::set<Digit> {digit}};
    }

    inline HandIndexer for_digits(Hand hand, const std::set<Digit> &digits) {
        return {std::set<Hand> {hand}, digits};
    }

    inline HandIndexer for_digit(Hand hand, Digit digit) {
        return {std::set<Hand> {hand}, std::set<Digit> {digit}};
    }

    class CCI_API_CLASS(HapticStatePatch) {
        public:
            HapticStatePatch() = default;
            virtual ~HapticStatePatch() = default;

            HapticStatePatch &with_forcefeedback_amplitude(const HandIndexer &indexer, float amplitude);
            HapticStatePatch &with_forcefeedback_position(const HandIndexer &indexer, float position);

            HapticStatePatch &with_vibration_amplitude(const HandIndexer &indexer, float amplitude);
            HapticStatePatch &with_vibration_effect(const HandIndexer &indexer, const VibrationEffect &effect);

            std::optional<float> forcefeedback_amplitude(Hand hand, Digit digit) const;
            std::optional<float> forcefeedback_position(Hand hand, Digit digit) const;

            std::optional<float> vibration_amplitude(Hand hand, Digit digit) const;
            std::optional<VibrationEffect> vibration_effect(Hand hand, Digit digit) const;

        private:
            HapticStatePatch &with_forcefeedback(const HandIndexer &indexer, const ForceFeedbackState &ffState);
            HapticStatePatch &with_vibration(const HandIndexer &indexer, const VibrationState &vibrationState);

            std::optional<ForceFeedbackState> forcefeedback(Hand hand, Digit digit) const;
            std::optional<VibrationState> vibration(Hand hand, Digit digit) const;

            std::array<HandState, 2> handStates;
    };
}