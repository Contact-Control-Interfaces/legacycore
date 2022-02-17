//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include <cstdint>
#include <deque>
#include <algorithm>
#include <sstream>
#include <tuple>

#include "core/haptics/dimension.h"
#include "core/haptics/frame.h"

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

    // Below pattern for HapticEffect adapted from https://stackoverflow.com/a/53112843/792779

    template <DimensionDerived D, DimensionDerived... Ds>
    class HapticEffect;

    template <DimensionDerived D>
    class HapticEffect<D> {
    public:
        // Friends with other specializations of HapticEffect<T> so we can access protected `dimension` field
        template <typename, typename...> friend class HapticEffect;

        explicit HapticEffect<D>(std::vector<D> dimension) : HapticEffect(dimension, slice(dimension)) { }

        virtual void pad_back(uint32_t pad_to_length) {
            uint32_t pad_count = std::max((uint32_t)0, (uint32_t)(pad_to_length - frames.size()));

            for (int i = 0; i < pad_count; i++)
                frames.push_back(DimensionedFrame<D>::get_zero());
        }

        virtual void pad_front(uint32_t pad_to_length) {
            uint32_t pad_count = std::max((uint32_t)0, (uint32_t)(pad_to_length - frames.size()));

            for (int i = 0; i < pad_count; i++)
                frames.push_front(DimensionedFrame<D>::get_zero());
        }

        HapticEffect<D> copy() {
            return HapticEffect<D>(*this);
        }

        HapticEffect<D> scale(double scalar) {
            this->scalar = scalar;

            return *this;
        }

        HapticEffect<D> delay(uint32_t length) {
            auto new_effect = this->copy();

            new_effect.pad_front(length + get_duration());

            return new_effect;
        }

        HapticEffect<D> then(HapticEffect<D> &other) {
            auto new_effect = this->copy();

            frames.insert(new_effect.frames.end(), other.frames.begin(), other.frames.end());

            return new_effect;
        }

        template <DimensionDerived T, DimensionDerived... Ts>
        HapticEffect<D, T, Ts...> then(HapticEffect<T, Ts...> &other) {
            HapticEffect<T, Ts...> other_copy(other);

            other_copy.pad_front(get_duration());

            return this->join(other_copy);
        }

        HapticEffect<D> repeat(uint32_t times) {
            auto new_effect = this->copy();

            if (times == 0)
                return new_effect;

            return new_effect.then(new_effect.repeat(times - 1));
        }

        template <DimensionDerived T, DimensionDerived... Ts>
        HapticEffect<D, T, Ts...> join(HapticEffect<T, Ts...> &other) {
            return HapticEffect<D, T, Ts...>(
                this->HapticEffect<D>::dimension,
                other.HapticEffect<T>::dimension,
                other.HapticEffect<Ts>::dimension...
            );
        }

        // Slices into frames
        // Assume dimension_elements sorted
        //TODO sort?
        std::vector<DimensionedFrame<D>> slice(std::vector<D> dimension_elements) {
            std::vector<DimensionedFrame<D>> sliced_frames;
            uint32_t current_frame_index = 0;
            uint32_t current_dimension_element_start_index = current_frame_index;

            for (auto dim_elem_it = dimension_elements.begin(); dim_elem_it != dimension_elements.end(); ++dim_elem_it) {
                D dim_elem = *dim_elem_it;
                uint32_t duration = dim_elem.get_duration();

                for (uint32_t local_frame_index = 0; local_frame_index < duration; local_frame_index++, current_frame_index++) {
                    sliced_frames.push_back(DimensionedFrame<D>(dim_elem.get_slice(local_frame_index)));
                }
            }

            return sliced_frames;
        }

        virtual uint32_t get_duration() const {
            return frames.size();
        }

    protected:
        std::deque<DimensionedFrame<D>> frames;

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

        DimensionedFrame<D> get_frame_at(uint32_t index) {
            return frames.at(index);
        }

        std::vector<D> dimension;
    private:
        explicit HapticEffect<D>(std::vector<D> dimension, std::vector<DimensionedFrame<D>> frames)
                : dimension(dimension), frames(frames.begin(), frames.end()) { }

        bool is_dimension_overlapped() {
            // TODO No longer have delay on effect
            return false;
        }
    };

    /*
     * then     +
     * join     +
     * delay    +
     * repeat   +
     * split
     * slice?
     */
    template <DimensionDerived D, DimensionDerived... Ds>
    class HapticEffect : public HapticEffect<D>, public HapticEffect<Ds>... {
    public:
        explicit HapticEffect(std::vector<D> first_dim, std::vector<Ds>... rest_dims)
               : HapticEffect<D>(first_dim), HapticEffect<Ds>(rest_dims)... {
            duration = get_max_dimension_duration<D, Ds...>();

            pad_back(duration);

            for (uint32_t i = 0; i < duration; i++)
                frames.push_back(get_frame(i));
        }

        HapticEffect<D, Ds...> copy() {
            return HapticEffect<D, Ds...>(*this);
        }

        void pad_back(uint32_t pad_to_length) override {
            _pad_back<D, Ds...>(pad_to_length);
        }

        void pad_front(uint32_t pad_to_length) override {
            _pad_front<D, Ds...>(pad_to_length);
        }

        HapticEffect<D, Ds...> delay(uint32_t length) {
            auto new_effect = this->copy();

            new_effect.pad_front(length + get_duration());

            return new_effect;
        }

        template <DimensionDerived T, DimensionDerived... Ts>
        HapticEffect<D, Ds..., T, Ts...> then(HapticEffect<T, Ts...> &other) {
            auto new_effect = other.copy();

            new_effect.pad_front(get_duration());

            return new_effect.join(*this);
        }

        HapticEffect<D, Ds...> repeat(uint32_t times) {
            auto new_effect = this->copy();

            if (times == 0)
                return new_effect;

            return new_effect.then(new_effect.repeat(times - 1));
        }

        template <DimensionDerived T, DimensionDerived... Ts>
        HapticEffect<D, Ds..., T, Ts...> join(HapticEffect<T, Ts...> &other) {
            return HapticEffect<D, Ds..., T, Ts...>(
                this->HapticEffect<D>::dimension,
                this->HapticEffect<Ds>::dimension...,
                other.HapticEffect<T>::dimension,
                other.HapticEffect<Ts>::dimension...
            );
        }

        std::tuple<HapticEffect<D>, HapticEffect<Ds>...> split() {
            return std::make_tuple(this->HapticEffect<D>::copy(), this->HapticEffect<Ds>::copy()...);
        }

        DimensionedFrame<D, Ds...> get_frame(uint32_t index) {
            return DimensionedFrame<D, Ds...>(
                this->HapticEffect<D>::get_frame_at(index).get_dimension_slice(),
                this->HapticEffect<Ds>::get_frame_at(index).get_dimension_slice()...
            );
        }

        std::vector<DimensionedFrame<D, Ds...>> get_frames() {
            return frames;
        }

        uint32_t get_duration() const override {
            return duration;
        }

    private:
        std::vector<DimensionedFrame<D, Ds...>> frames;
        uint32_t duration;

        template <DimensionDerived T, DimensionDerived... Ts>
        void _pad_front(uint32_t pad_to_length) {
            HapticEffect<T>::pad_front(pad_to_length);

            if constexpr (sizeof...(Ts) != 0)
                _pad_front<Ts...>(pad_to_length);
        }

        template <DimensionDerived T, DimensionDerived... Ts>
        void _pad_back(uint32_t pad_to_length) {
            HapticEffect<T>::pad_back(pad_to_length);

            if constexpr (sizeof...(Ts) != 0)
                _pad_back<Ts...>(pad_to_length);
        }

        template<DimensionDerived T, DimensionDerived ...Ts>
        uint32_t get_max_dimension_duration() const {
            if constexpr (sizeof...(Ts) == 0) {
                // This is the base case where Ts is empty and we just have template arg T
                // Since the condition is a constexpr, the else block is completely discarded at compile-time when it evaluates to true
                // avoiding the base case issue of unpacking empty Ts
                return this->HapticEffect<T>::get_duration();
            } else { // This else needs to be here explicitly to avoid issues deducing template argument T for base case
                return std::max(
                    // Get max for dimension for type T
                    this->HapticEffect<T>::get_duration(),
                    // Recurse on remaining dimension types Ts
                    get_max_dimension_duration<Ts...>()
                );
            }
        }
    };
}