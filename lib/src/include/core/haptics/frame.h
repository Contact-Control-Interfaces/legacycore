//
// Created by john_contactci on 2/2/2022.
//

#pragma once

#include "dimension.h"
#include "core/haptics/effects/player.h"

namespace contactci::core::haptics {

    class Frame {
        virtual void play() const = 0;
    };

    class NullFrame : public Frame {
        void play() const override { }
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame;

    template<DimensionDerived D>
    class DimensionedFrame<D> : public Frame {
    public:
        virtual TypedDimensionSlice<D> get_dimension_slice() const {
            return dimension_slice;
        }

        virtual void set_dimension_slice(const TypedDimensionSlice<D> slice) {
            dimension_slice = slice;
        }

        void play() const override {
            DimensionedSlicePlayer<D>::play(get_dimension_slice());
        }

    private:
        TypedDimensionSlice<D> &dimension_slice;
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame : public DimensionedFrame<D>, public DimensionedFrame<Ds...> {
    public:
        template<typename T>
        TypedDimensionSlice<T> get_dimension_slice() const {
            return this->DimensionedFrame<T>::get_dimension();
        }

        template<typename T>
        void set_dimension_slice(const TypedDimensionSlice<T> dimension) {
            this->DimensionedFrame<T>::set_dimension(dimension);
        }

        void play() const override {
            play_dimensions();
        }

    private:
        template<typename T, typename ...Ts>
        void play_dimensions() const {
            if constexpr (sizeof...(Ts) == 0) {
                // This is the base case where Ts is empty and we just have template arg T
                // Since the condition is a constexpr, the else block is completely discarded at compile-time when it evaluates to true
                // avoiding the base case issue of unpacking empty Ts
                DimensionedSlicePlayer<T>::play(this->DimensionedFrame<T>::get_dimension_slice());
            } else { // This else needs to be here explicitly to avoid issues deducing template argument T for base case
                // Recurse on remaining dimension types Ts
                play_dimensions<Ts...>();
            }
        }
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class NullDimensionedFrame;

    template<DimensionDerived D>
    class NullDimensionedFrame<D> : public DimensionedFrame<D>, public NullFrame {
    public:
        void play() const override { }
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class NullDimensionedFrame : public DimensionedFrame<D>, public DimensionedFrame<Ds...>, public NullFrame {
    public:
        void play() const override { }
    };
}