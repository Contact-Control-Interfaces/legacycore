//
// Created by john_contactci on 2/3/2022.
//


// Headers will be ignored if they're not included anywhere, so this file also us to properly test compilation
#include "core/haptics/dimension.h"

#include "core/haptics/effects/dimensioned_haptic_effect.h"

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension : public TemporalDimension<VibrationDimension> {
public:
    VibrationDimension(uint32_t delay, uint32_t duration)
        : TemporalDimension<VibrationDimension>(delay, duration) { }

    DimensionSlice get_slice(uint32_t offset) override {
        return {};
    }
};

class ForceFeedbackDimension : public TemporalDimension<ForceFeedbackDimension> {
public:
    ForceFeedbackDimension(uint32_t delay, uint32_t duration)
        : TemporalDimension<ForceFeedbackDimension>(delay, duration) { }

    DimensionSlice get_slice(uint32_t offset) override {
        return {};
    }
};

void tes() {
    auto vib_dim = VibrationDimension(2, 5);
    auto ff_dim = ForceFeedbackDimension(3, 2);

    DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect;

    effect.set_dimension(std::vector<VibrationDimension> {vib_dim});
    effect.add_to_dimension(vib_dim); // this should throw an overlap exception

    effect.set_dimension(std::vector<ForceFeedbackDimension> {ff_dim});
    effect.set_dimension(std::vector<ForceFeedbackDimension> {ff_dim});

    effect.get_duration();


    std::vector<ForceFeedbackDimension> works_fine = effect.get_dimension<ForceFeedbackDimension>();
    //std::vector<int> blows_up = effect.get_dimension<int>();
}