//
// Created by john_contactci on 2/2/2022.
//

#pragma once

#include "dimension.h"

namespace contactci::core::haptics {


    class NullFrame {
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame;

    template<DimensionDerived D>
    class DimensionedFrame<D> {
    public:
        static DimensionedFrame<D> get_zero();
        explicit DimensionedFrame<D>(TypedDimensionSlice<D> &dimension_slice) : dimension_slice(dimension_slice) { }

        virtual TypedDimensionSlice<D> &get_dimension_slice() const {
            return dimension_slice;
        }

        virtual void set_dimension_slice(const TypedDimensionSlice<D> &slice) {
            dimension_slice = slice;
        }

    private:
        TypedDimensionSlice<D> &dimension_slice;
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame : public DimensionedFrame<D>, public DimensionedFrame<Ds>... {
    public:
        explicit DimensionedFrame(TypedDimensionSlice<D> &first_slice, TypedDimensionSlice<Ds> &...rest_slices)
            : DimensionedFrame<D>(first_slice), DimensionedFrame<Ds>(rest_slices)... { }

        template<typename T>
        TypedDimensionSlice<T> &get_dimension_slice() const {
            return this->DimensionedFrame<T>::get_dimension_slice();
        }

        template<typename T>
        void set_dimension_slice(const TypedDimensionSlice<T> &slice) {
            this->DimensionedFrame<T>::set_dimension_slice(slice);
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

    template<DimensionDerived D>
    DimensionedFrame<D> DimensionedFrame<D>::get_zero() {
        return DimensionedFrame<D>(TypedDimensionSlice<D>::get_zero());
    }
}
