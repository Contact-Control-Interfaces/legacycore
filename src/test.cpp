//
// Created by john_contactci on 2/3/2022.
//


// Headers will be ignored if they're not included anywhere, so this file also us to properly test compilation
#include "core/haptics/dimension.h"

#include "core/haptics/effects/dimensioned_haptic_effect.h"

#include "core/haptics/effects/player.h"

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension;
class ForceFeedbackDimension;

class VibrationSlice : public TypedDimensionSlice<VibrationDimension> {

};

class ForceFeedbackSlice : public TypedDimensionSlice<ForceFeedbackDimension> {

};

class VibrationDimension : public TemporalDimension<VibrationDimension> {
public:
    VibrationDimension(uint32_t delay, uint32_t duration)
        : TemporalDimension<VibrationDimension>(delay, duration) { }

    TypedDimensionSlice<VibrationDimension> get_slice(uint32_t offset) override {
        return VibrationSlice();
    }
};

class ForceFeedbackDimension : public TemporalDimension<ForceFeedbackDimension> {
public:
    ForceFeedbackDimension(uint32_t delay, uint32_t duration)
        : TemporalDimension<ForceFeedbackDimension>(delay, duration) { }

    TypedDimensionSlice<ForceFeedbackDimension> get_slice(uint32_t offset) override {
        return ForceFeedbackSlice();
    }
};

template<DimensionDerived T>
struct assert_false : std::false_type
{ };

template<DimensionDerived T>
void DimensionedSlicePlayer<T>::play(TypedDimensionSlice<T> slice) {
    static_assert(assert_false<T>::value, "No specialization available to play haptic effect of dimension");
}

template<>
void DimensionedSlicePlayer<VibrationDimension>::play(TypedDimensionSlice<VibrationDimension> slice) {
    // Use slice.effectCode do stuff
}

template<>
void DimensionedSlicePlayer<ForceFeedbackDimension>::play(TypedDimensionSlice<ForceFeedbackDimension> slice) {
    // Use slice.amplitude do stuff
}

void tes() {

    //DimensionedPlayer<VibrationDimension>::play();
    //DimensionedHapticEffect<VibrationDimension> effect;
    //player.play(effect);

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