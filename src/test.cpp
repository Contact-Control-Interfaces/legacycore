//
// Created by john_contactci on 2/3/2022.
//


// Headers will be ignored if they're not included anywhere, so this file also us to properly test compilation
#include "core/haptics/dimension.h"

#include "core/haptics/effects/dimensioned_haptic_effect.h"

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension : public Dimension {
public:
    DimensionSlice get_slice(uint32_t offset) override {
        return {};
    }
};

class ForceFeedbackDimension : public Dimension {
public:
    DimensionSlice get_slice(uint32_t offset) override {
        return {};
    }
};

void tes() {
    DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect;

    effect.set_dimension(std::vector<VibrationDimension> {VibrationDimension(), VibrationDimension()});
    effect.set_dimension(std::vector<ForceFeedbackDimension> {ForceFeedbackDimension()});
    effect.set_dimension(std::vector<ForceFeedbackDimension> {ForceFeedbackDimension()});


    std::vector<ForceFeedbackDimension> works_fine = effect.get_dimension<ForceFeedbackDimension>();
    //std::vector<int> blows_up = effect.get_dimension<int>();
}