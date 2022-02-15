//
// Created by john_contactci on 2/3/2022.
//

#include <core/haptics/dimension.h>
#include <core/haptics/effects/dimensioned_haptic_effect.h>
#include <core/haptics/effects/player.h>

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension;
class ForceFeedbackDimension;

class VibrationSlice : public TypedDimensionSlice<VibrationDimension> {
public:
    static VibrationSlice ZERO;
    explicit VibrationSlice(uint8_t effect) : effect(effect) { }

    uint8_t get_effect() {
        return effect;
    }

    TypedDimensionSlice<VibrationDimension> &get_zero() const override {
        return ZERO;
    }
private:
    uint8_t effect;
};

VibrationSlice VibrationSlice::ZERO(0);

class ForceFeedbackSlice : public TypedDimensionSlice<ForceFeedbackDimension> {
public:
    static ForceFeedbackSlice ZERO;

    explicit ForceFeedbackSlice(float amplitude) : amplitude(amplitude) { }

    float get_amplitude() {
        return amplitude;
    }

    TypedDimensionSlice<ForceFeedbackDimension> &get_zero() const override {
        return ZERO;
    }

private:
    float amplitude; // 0 - 1
};

ForceFeedbackSlice ForceFeedbackSlice::ZERO(0);

class VibrationDimension : public Dimension<VibrationDimension> {
public:
    explicit VibrationDimension(uint32_t duration)
            : Dimension<VibrationDimension>(duration) { }

    TypedDimensionSlice<VibrationDimension> &get_slice(uint32_t offset) override {
        return VibrationSlice::ZERO;
    }
};

class ForceFeedbackDimension : public Dimension<ForceFeedbackDimension> {
public:
    explicit ForceFeedbackDimension(uint32_t duration)
            : Dimension<ForceFeedbackDimension>(duration) { }

    TypedDimensionSlice<ForceFeedbackDimension> &get_slice(uint32_t offset) override {
        return ForceFeedbackSlice::ZERO;
    }
};

template<>
void DimensionedSlicePlayer<VibrationDimension>::play(TypedDimensionSlice<VibrationDimension> &slice) {
    // Use slice.effectCode do stuff
}

template<>
void DimensionedSlicePlayer<ForceFeedbackDimension>::play(TypedDimensionSlice<ForceFeedbackDimension> &slice) {
    // Use slice.amplitude do stuff
}

int main() {

    //DimensionedPlayer<VibrationDimension>::play();
    //DimensionedHapticEffect<VibrationDimension> effect;
    //player.play(effect);

    // TODO check scale
    /*std::cout << "Creating sequence" << std::endl;
    HapticEffectSequence sequence(HapticEffect::ZERO);
    std::cout << sequence.get_duration() << std::endl;
    sequence.scale(2);
    std::cout << sequence.get_duration() << std::endl;*/


    auto vib_dim = VibrationDimension(5);
    auto ff_dim = ForceFeedbackDimension(2);

    DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect(
        std::vector<VibrationDimension> {vib_dim},
        std::vector<ForceFeedbackDimension> {ff_dim}
    );

//
//    effect.get_duration();

    auto seq = HapticEffectSequence(reinterpret_cast<HapticEffect&>(effect));

//    HapticEffectSequence::Iterator it(seq);
//
//    auto asd = it + 3;
//    it++;


    std::vector<ForceFeedbackDimension> works_fine = effect.get_dimension<ForceFeedbackDimension>();
    //std::vector<int> blows_up = effect.get_dimension<int>();

    return 0;
}