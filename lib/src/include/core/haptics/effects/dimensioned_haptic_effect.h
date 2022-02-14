//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include <algorithm>
#include <sstream>

#include "core/haptics/dimension.h"
#include "core/haptics/effects/haptic_effect.h"

#include "core/haptics/effects/player.h"

namespace contactci::core::haptics::effects {

    class OverlappedDimensionException : public std::runtime_error {
    public:
        explicit OverlappedDimensionException(const std::string &message) : std::runtime_error(message) { }
        explicit OverlappedDimensionException(const std::type_info &dimension_type) : OverlappedDimensionException(
            (
                std::ostringstream("Overlapping haptic effects for dimension of type ")
                    << dimension_type.name()
            ).str()
        ) { }
    };

    // Below pattern for DimensionedHapticEffect adapted from https://stackoverflow.com/a/53112843/792779

    template <DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect;

    template <DimensionDerived D>
    class DimensionedHapticEffect<D> : public TypedHapticEffect<DimensionedHapticEffect<D>> {
    public:
        //explicit DimensionedHapticEffect<D>(std::vector<D> dimension) : frames(slice(dimension)) { }

        DimensionedHapticEffect<D> scale(double scalar) override {
            this->scalar = scalar;

            return *this;
        }

        //TODO Probably dont need to redefine chain in subtypes
        //Maybe no pure virtual and just regular virtual
        HapticEffectSequence chain(HapticEffect &other) override {
            return HapticEffectSequence(*this).chain(other);
        }

        DimensionedFrame<D> &get_current_frame() override {
            return current_frame;
        }

        // Slices into frames
        // Assume dimension_elements sorted
        std::vector<DimensionedFrame<D>> slice(std::vector<D> dimension_elements) {
            std::vector<DimensionedFrame<D>> frames;

            typename std::vector<D>::iterator current_dimension_element = dimension_elements.begin();

            for (uint32_t i = 0; ; i++) {
                DimensionedFrame<D> frame;

                frame.template set_dimension_slice(current_dimension_element->get_slice(i));

                if (i == current_dimension_element->get_duration()) {
                    current_dimension_element++;
                }
            }

            return frames;
        }

        void move_next_frame() override {
//            current_frame_index++;
//
//            // getdimensionelement for index
//            if (current_frame_index > current_frame_dimension_element->get_length()) {
//                current_frame_dimension_element++;
//            }
//
//            current_frame = DimensionedFrame<D>();
//            // TODO Should Dimension really even know about it's own delay? If so, should it handle translating
//            // global frame index to relative? i.e. current_frame_index - current_frame_dimension_element->get_delay()
//            current_frame.set_dimension_slice(
//                current_frame_dimension_element->get_slice(current_frame_index)
//            );
        }

        uint32_t get_duration() const override {
            auto dimension_length_comparer = [](D a, D b) { return a.get_duration() < b.get_duration(); };
            auto max_iterator = std::max_element(std::begin(dimension), std::end(dimension), dimension_length_comparer);

            return max_iterator->get_length();
        }

    protected:
        virtual std::vector<D> get_dimension() {
            return dimension;
        }

        virtual void set_dimension(const std::vector<D> dimension_vec) {
            dimension = dimension_vec;

            validate_dimension();

            frames = slice(dimension);
        }

        virtual void add_to_dimension(const D &dimension_element) {
            dimension.push_back(dimension_element);

            validate_dimension();

            frames = slice(dimension);
        }

        virtual void validate_dimension() {
            // TODO No longer have delay on effect

            // TODO we should allow the delay of effect to overlap the the duration of another
            // Sort by delay ascending
//            std::sort(std::begin(dimension), std::end(dimension), [](D a, D b) {
//                return a.get_delay() < b.get_delay();
//            });

            if (is_dimension_overlapped()) {
                throw OverlappedDimensionException(typeid(D));
            }
        }

    private:
        std::vector<DimensionedFrame<D>> frames {};
        std::vector<D> dimension {};

        //TODO figure out how to initialize this
        DimensionedFrame<D> current_frame{};

        bool is_dimension_overlapped() {
            // TODO No longer have delay on effect
            return false;
        }
    };

    template <DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect : public DimensionedHapticEffect<D>, public DimensionedHapticEffect<Ds...> {
    public:

        //TODO doesn't work as expected:
//        DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect(
//                std::vector<VibrationDimension> {vib_dim},
//        std::vector<ForceFeedbackDimension> {ff_dim}
//        );

//        explicit DimensionedHapticEffect(std::vector<Ds> ...dimensions) : DimensionedHapticEffect<Ds>(dimensions)... {
//        }

        template <typename V>
        std::vector<V> get_dimension() {
            return this->DimensionedHapticEffect<V>::get_dimension();
        }

        template <typename V>
        void set_dimension(const std::vector<V> dimension) {
            this->DimensionedHapticEffect<V>::set_dimension(dimension);
        }

        template <typename V>
        void add_to_dimension(const V &dimension_element) {
            this->DimensionedHapticEffect<V>::add_to_dimension(dimension_element);
        }

        DimensionedFrame<D, Ds...> &get_current_frame() override {
            return current_frame;
        }

        uint32_t get_duration() const override {
            return get_max_dimension_duration<D, Ds...>();
        }

    private:
        DimensionedFrame<D, Ds...> current_frame{};

        template<typename T, typename ...Ts>
        uint32_t get_max_dimension_duration() const {
            if constexpr (sizeof...(Ts) == 0) {
                // This is the base case where Ts is empty and we just have template arg T
                // Since the condition is a constexpr, the else block is completely discarded at compile-time when it evaluates to true
                // avoiding the base case issue of unpacking empty Ts
                return this->DimensionedHapticEffect<T>::get_duration();
            } else { // This else needs to be here explicitly to avoid issues deducing template argument T for base case
                return std::max(
                    // Get max for dimension for type T
                    this->DimensionedHapticEffect<T>::get_duration(),
                    // Recurse on remaining dimension types Ts
                    get_max_dimension_duration<Ts...>()
                );
            }
        }
    };
}