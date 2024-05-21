//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <ccic/haptic_state.h>
#include <ccic/lib_defs.h>

#include <string>
#include <memory>
#include <optional>
#include <variant>

namespace contactci {
    class CCI_API_CLASS(HapticStateManager) {
    public:
        HapticStateManager(const std::string &memoryName, const std::string &eventName);

        ~HapticStateManager();

        const HapticState &get_left_haptic_state() const;

        const HapticState &get_right_haptic_state() const;

        bool wait_for_state_change(int timeout_ms) const;

    protected:
        class Implementation;

        HapticStateManager(std::unique_ptr<Implementation> &&implementation);

        std::unique_ptr<Implementation> implementation;
    };

    class CCI_API_CLASS(MutableHapticStateManager) : public HapticStateManager {
    public:
        MutableHapticStateManager(const std::string &memoryName, const std::string &eventName);

        HapticState &get_left_haptic_state();

        HapticState &get_right_haptic_state();

        void signal_haptic_state_changed();
    };

    namespace haptic_state {
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
        enum class DigitStateMember : std::size_t {
            ForceFeedback = 0,
            Vibration = 1
        };

        enum class Digit : std::size_t {
            Thumb = 0,
            Index = 1,
            Middle = 2,
            Ring = 3,
            Little = 4
        };

        class CCI_API_CLASS(HapticStatePatch) {
        public:
            HapticStatePatch() = default;
            virtual ~HapticStatePatch() = default;

            template<Digit digit>
            HapticStatePatch &with_forcefeedback_amplitude(float amplitude);

            template<Digit digit>
            HapticStatePatch &with_forcefeedback_position(float position);

            template<Digit digit>
            HapticStatePatch &with_vibration_amplitude(float amplitude);

            template<Digit digit>
            HapticStatePatch &with_vibration_effect(const VibrationEffect &effect);

            template<Digit digit>
            std::optional<float> forcefeedback_amplitude();

            template<Digit digit>
            std::optional<float> forcefeedback_position();

            template<Digit digit>
            std::optional<float> vibration_amplitude();

            template<Digit digit>
            std::optional<VibrationEffect> vibration_effect();

        private:
            template<Digit digit>
            HapticStatePatch &with_forcefeedback(const ForceFeedbackState &ffState);

            template<Digit digit>
            HapticStatePatch &with_vibration(const VibrationState &vibrationState);

            template<Digit digit>
            const std::optional<DigitState> &get_digit_state() const;

            template<Digit digit>
            const std::optional<ForceFeedbackState> &get_forcefeedback() const;

            template<Digit digit>
            const std::optional<VibrationState> &get_vibration() const;

            std::tuple<
                std::optional<DigitState>,
                std::optional<DigitState>,
                std::optional<DigitState>,
                std::optional<DigitState>,
                std::optional<DigitState>
            > digitStates;
        };
    }
}