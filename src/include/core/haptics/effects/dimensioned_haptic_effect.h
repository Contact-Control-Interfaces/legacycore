//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include <algorithm>
#include <sstream>

#include "core/haptics/dimension.h"
#include "core/haptics/effects/haptic_effect.h"
#include "core/haptics/effects/haptic_effect_sequence.h"

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
        DimensionedHapticEffect<D> scale(double scalar) override {
            this->scalar = scalar;

            return *this;
        }

        HapticEffectSequence chain(HapticEffect &other) override {
            return HapticEffectSequence(*this).chain(other);
        }

        DimensionedFrame<D> &get_current_frame() override {
            return current_frame;
        }

        void move_next_frame() override {
            // TODO
        }

        uint32_t get_duration() override {
            // TODO
            return 0;
        }

    protected:
        virtual std::vector<D> get_dimension() {
            return dimension;
        }

        virtual void set_dimension(const std::vector<D> dimension_vec) {
            dimension = dimension_vec;

            validate_dimension();
        }

        virtual void add_to_dimension(const D &dimension_element) {
            dimension.push_back(dimension_element);

            validate_dimension();
        }

        virtual void validate_dimension() {
            // Sort by delay ascending
            std::sort(std::begin(dimension), std::end(dimension), [](D a, D b) {
                return a.get_delay() < b.get_delay();
            });

            if (is_dimension_overlapped()) {
                throw OverlappedDimensionException(typeid(D));
            }
        }

    private:
        std::vector<D> dimension;
        DimensionedFrame<D> current_frame;
        bool is_dimension_overlapped() {
            typename std::vector<D>::size_type size = dimension.size();

            if (size < 2) {
                return false;
            }

            for (int i = 1; i < size; i++) {
                const D &previous = dimension[i - 1];
                const D &current = dimension[i];
                const uint32_t previous_end = previous.get_length();

                if (previous_end > current.get_delay()) {
                    return true;
                }
            }

            return false;
        }
    };

    template <DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect : public DimensionedHapticEffect<D>, public DimensionedHapticEffect<Ds...> {
    public:
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

    private:
        DimensionedFrame<D, Ds...> current_frame;
    };
}