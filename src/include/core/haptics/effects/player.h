//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/haptics/dimension.h"

namespace contactci::core::haptics {

    template<DimensionDerived S>
    class DimensionSlicePlayer {
    public:
        virtual void play(TypedDimensionSlice<S> &slice) = 0;
    };

    template<DimensionDerived S, DimensionDerived... Ss>
    class DimensionedPlayer : public DimensionSlicePlayer<S>, public DimensionSlicePlayer<Ss...> {
    public:
        template<DimensionDerived T>
        void play(TypedDimensionSlice<T> &slice) override {
            this->DimensionedPlayer<TypedDimensionSlice<T>>::play(slice);
        }
    };
}