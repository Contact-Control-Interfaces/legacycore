//
// Created by john_contactci on 2/2/2022.
//

#pragma once

#include "dimension.h"

namespace contactci::core::haptics {

    class Frame {

    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame;

    template<DimensionDerived D>
    class DimensionedFrame<D> : public Frame {
    public:
        virtual TypedDimensionSlice<D> get_dimension_slice() {
            return dimension_slice;
        }

        virtual void set_dimension_slice(const TypedDimensionSlice<D> slice) {
            dimension_slice = slice;
        }

    private:
        TypedDimensionSlice<D> dimension_slice;
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedFrame : public DimensionedFrame<D>, public DimensionedFrame<Ds...> {
    public:
        template<typename T>
        TypedDimensionSlice<T> get_dimension_slice() {
            return this->DimensionedFrame<T>::get_dimension();
        }

        template<typename T>
        void set_dimension_slice(const TypedDimensionSlice<T> dimension) {
            this->DimensionedFrame<T>::set_dimension(dimension);
        }
    };
}