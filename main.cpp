//
// Created by john_contactci on 2/3/2022.
//

#include <core/haptics/dimension.h>
#include <core/haptics/effects/haptic_effect.h>
#include <core/haptics/effects/player.h>

#include <comms/communicator.h>

#include <iostream>
#include <list>

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension;
class ForceFeedbackDimension;
class PressureDimension;

class VibrationSlice : public TypedDimensionSlice<VibrationDimension> {
public:
    static VibrationSlice ZERO;
    explicit VibrationSlice(uint8_t effect) : effect(effect) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    uint8_t get_effect() {
        return effect;
    }
private:
    uint8_t effect;
};

template <>
bool VibrationSlice::operator==<VibrationSlice>(const VibrationSlice &another) const {
    return effect == another.effect;
}

VibrationSlice VibrationSlice::ZERO(0);

class ForceFeedbackSlice : public TypedDimensionSlice<ForceFeedbackDimension> {
public:
    static ForceFeedbackSlice ZERO;

    explicit ForceFeedbackSlice(float amplitude) : amplitude(amplitude) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    float get_amplitude() {
        return amplitude;
    }
private:
    float amplitude; // 0 - 1
};

template <>
bool ForceFeedbackSlice::operator==<ForceFeedbackSlice>(const ForceFeedbackSlice &another) const {
    return amplitude == another.amplitude;
}

ForceFeedbackSlice ForceFeedbackSlice::ZERO(0);

class PressureSlice : public TypedDimensionSlice<PressureDimension> {
public:
    static PressureSlice ZERO;


    explicit PressureSlice(float amplitude) : amplitude(amplitude) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    float get_amplitude() {
        return amplitude;
    }
private:
    float amplitude; // 0 - 1
};

template <>
bool PressureSlice::operator==<PressureSlice>(const PressureSlice &another) const {
    return amplitude == another.amplitude;
}

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

class DebugCommunicator : public contactci::comms::Communicator {
public:
    void start_frame() override {
        /* TODO */
    }
    void end_frame() override {
        std::cout << std::endl;
    }
    void send(uint8_t byte) override {
        std::cout << (byte != 0 ? '#' : '|');
    }
};

template <>
char Dimension<PressureDimension>::get_representation() {
    return '|';
}

template <>
char Dimension<VibrationDimension>::get_representation() {
    return '~';
}

template <>
char Dimension<ForceFeedbackDimension>::get_representation() {
    return '#';
}

template<>
void DimensionedSlicePlayer<VibrationDimension>::play(contactci::comms::Communicator &comms, TypedDimensionSlice<VibrationDimension> &slice) {
    char output = ((VibrationSlice&)slice) == VibrationSlice::ZERO ? '#' : VibrationDimension::get_representation();

    comms.send(output);
}

template<>
void DimensionedSlicePlayer<ForceFeedbackDimension>::play(contactci::comms::Communicator &comms, TypedDimensionSlice<ForceFeedbackDimension> &slice) {
    char output = ((ForceFeedbackSlice&)slice) == ForceFeedbackSlice::ZERO ? '#' : ForceFeedbackDimension::get_representation();

    comms.send(output);
}

template<>
void DimensionedSlicePlayer<PressureDimension>::play(contactci::comms::Communicator &comms, TypedDimensionSlice<PressureDimension> &slice) {
    char output = ((PressureSlice&)slice) == PressureSlice::ZERO ? '#' : PressureDimension::get_representation();

    comms.send(output);
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


//    HapticEffect<VibrationDimension>::flat(52).repeat(10);
//
//
//    HapticEffect<VibrationDimension>::ramp


    //TODO handle merging when same dimension type is provided multiple times, e.g. HapticEffect<VibrationDimension, ForceFeedbackDimension, VibrationDimension>

    HapticEffect<VibrationDimension, ForceFeedbackDimension> effect(
        std::list<VibrationDimension> {vib_dim, vib_dim2},
        std::list<ForceFeedbackDimension> {ff_dim}
    );

    HapticEffect<VibrationDimension, ForceFeedbackDimension> effect2(
            std::list<VibrationDimension> {vib_dim, vib_dim2},
            {}
    );

    HapticEffect<VibrationDimension> part1(std::list<VibrationDimension> {vib_dim, vib_dim2});
    HapticEffect<ForceFeedbackDimension> part2(std::list<ForceFeedbackDimension> {ff_dim});

    HapticEffect<VibrationDimension, ForceFeedbackDimension> stacked = part1.join(part2);

    HapticEffect<PressureDimension> pressure_effect(std::list<PressureDimension> {press});

    HapticEffect<VibrationDimension, ForceFeedbackDimension, PressureDimension> asdf = stacked.join(pressure_effect);




    auto temp = part1.then(part2);
    HapticEffect<VibrationDimension, ForceFeedbackDimension, PressureDimension> temp2 = temp.join(pressure_effect);

    HapticEffect<VibrationDimension> test;
    HapticEffect<ForceFeedbackDimension> test1;
    HapticEffect<PressureDimension> test2;

    std::tie(test, test1, test2) = temp2.split();
    std::cout << test.get_duration() << std::endl;

    auto scale_test = test.scale(0.5);

    std::cout << scale_test.get_duration() << std::endl;

    auto vs = VibrationSlice(52);
    auto fs = ForceFeedbackSlice(175);
    auto ps = PressureSlice(0);

    HapticEffect<VibrationDimension, ForceFeedbackDimension, PressureDimension> temp3(vs, fs, ps);

    DebugCommunicator comms;

    auto temp4 = temp3.then(HapticEffect<VibrationDimension, ForceFeedbackDimension, PressureDimension>().delay(5));

    EffectPlayer::play(comms, temp4);

    //
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