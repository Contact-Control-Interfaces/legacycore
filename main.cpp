//
// Created by john_contactci on 2/3/2022.
//

#include <core/haptics/dimension.h>
#include <core/haptics/effects/haptic_effect.h>
#include <core/haptics/effects/player.h>

#include <iostream>

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension;
class ForceFeedbackDimension;
class PressureDimension;

class VibrationSlice : public TypedDimensionSlice<VibrationDimension> {
public:
    static VibrationSlice ZERO;
    explicit VibrationSlice(uint8_t effect) : effect(effect) { }

    uint8_t get_effect() {
        return effect;
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
private:
    float amplitude; // 0 - 1
};

ForceFeedbackSlice ForceFeedbackSlice::ZERO(0);

class PressureSlice : public TypedDimensionSlice<PressureDimension> {
public:
    static PressureSlice ZERO;

    explicit PressureSlice(float amplitude) : amplitude(amplitude) { }

    float get_amplitude() {
        return amplitude;
    }
private:
    float amplitude; // 0 - 1
};

PressureSlice PressureSlice::ZERO(0);

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

class PressureDimension : public Dimension<PressureDimension> {
public:
    explicit PressureDimension(uint32_t duration)
            : Dimension<PressureDimension>(duration) { }

    TypedDimensionSlice<PressureDimension> &get_slice(uint32_t offset) override {
        return PressureSlice::ZERO;
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

template<>
TypedDimensionSlice<VibrationDimension> &TypedDimensionSlice<VibrationDimension>::get_zero() {
    return VibrationSlice::ZERO;
}

template<>
TypedDimensionSlice<ForceFeedbackDimension> &TypedDimensionSlice<ForceFeedbackDimension>::get_zero() {
    return ForceFeedbackSlice::ZERO;
}

template<>
TypedDimensionSlice<PressureDimension> &TypedDimensionSlice<PressureDimension>::get_zero() {
    return PressureSlice::ZERO;
}

int main() {

    //DimensionedPlayer<VibrationDimension>::play();
    //HapticEffect<VibrationDimension> effect;
    //player.play(effect);

    auto vib_dim = VibrationDimension(5);
    auto vib_dim2 = VibrationDimension(8);
    auto ff_dim = ForceFeedbackDimension(2);
    auto press = PressureDimension(5);


    //TODO handle merging when same dimension type is provided multiple times, e.g. HapticEffect<VibrationDimension, ForceFeedbackDimension, VibrationDimension>

    HapticEffect<VibrationDimension, ForceFeedbackDimension> effect(
        std::vector<VibrationDimension> {vib_dim, vib_dim2},
        std::vector<ForceFeedbackDimension> {ff_dim}
    );

    HapticEffect<VibrationDimension, ForceFeedbackDimension> effect2(
            std::vector<VibrationDimension> {vib_dim, vib_dim2},
            {}
    );

    HapticEffect<VibrationDimension> part1(std::vector<VibrationDimension> {vib_dim, vib_dim2});
    HapticEffect<ForceFeedbackDimension> part2(std::vector<ForceFeedbackDimension> {ff_dim});

    HapticEffect<VibrationDimension, ForceFeedbackDimension> stacked = part1.stack(part2);

    HapticEffect<PressureDimension> pressure_effect(std::vector<PressureDimension> {press});

    HapticEffect<VibrationDimension, ForceFeedbackDimension, PressureDimension> asdf = stacked.stack(pressure_effect);



    // Below throws error for multiple initializations of HapticEffect<VibrationDimension> since it was included twice
    // I suppose this is good since it prevents specifying the same dimension twice
    //HapticEffect<VibrationDimension, ForceFeedbackDimension, VibrationDimension, ForceFeedbackDimension> stacked2 = effect.stack(effect2);


    //HapticEffect<VibrationDimension, ForceFeedbackDimension> effect3 = effect.chain(effect2);

    //auto seq = HapticEffectSequence(reinterpret_cast<HapticEffect&>(effect));


//    HapticEffectSequence::Iterator it(seq);
//
//    auto asd = it + 3;
//    it++;

    //std::vector<int> blows_up = effect.get_dimension<int>();

    return 0;
}