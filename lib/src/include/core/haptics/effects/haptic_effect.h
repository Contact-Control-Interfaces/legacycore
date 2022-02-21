//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include <cstdint>
#include <list>
#include <algorithm>
#include <sstream>
#include <tuple>

#include <iterator>

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

        HapticEffect() = default;
        explicit HapticEffect<D>(std::list<DimensionedFrame<D>> frames) : frames(frames.begin(), frames.end()) { }
        explicit HapticEffect<D>(std::list<D> dimension) : HapticEffect(slice_frames(dimension)) { }

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
            auto new_effect = this->copy();

            new_effect.self_scale(scalar);

            return new_effect;
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
                this->HapticEffect<D>::frames,
                other.HapticEffect<T>::frames,
                other.HapticEffect<Ts>::frames...
            );
        }

        virtual uint32_t get_duration() const {
            return frames.size();
        }

        const std::list<DimensionedFrame<D>> get_frames() const {
            return frames;
        }

    protected:
        std::list<DimensionedFrame<D>> frames;

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

        void self_scale(double scalar) {
            const uint32_t duration = get_duration();
            const auto new_frames_count = static_cast<int32_t>((scalar - 1.0) * duration);

            const uint32_t space_between = duration / abs(new_frames_count);
            const uint32_t initial_offset = space_between / 2;

            auto current_it = frames.begin();
            std::advance(current_it, initial_offset);

            if (new_frames_count > 0) {
                int i = 0;

                do {
                    frames.insert(current_it, *current_it);

                    std::advance(current_it, space_between + 1); // +1 to account for newly inserted frame

                    i++;
                } while (i < new_frames_count);
            } else if (new_frames_count < 0) {
                // new_frames_count is negative, so we increment up to 0 in our loop
                int i = new_frames_count;

                do {
                    current_it = frames.erase(current_it);

                    // ######
                    //  #####
                    //  ^

                    std::advance(current_it, space_between - 1); // 1 to account for removed frame shrinking the list

                    i++;
                } while (i < 0);
            } // else, new_frames_count is 0 so do nothing;
            // can't use just `else` when `new_frames_count` is negative since we use do-while:
            // the loop condition isn't checked until after the body
        }

        // Slices into frames
        // Assume dimension_elements sorted
        //TODO sort?
        std::list<DimensionedFrame<D>> slice_frames(std::list<D> dimension_elements) {
            std::list<DimensionedFrame<D>> sliced_frames;
            uint32_t current_frame_index = 0;

            for (auto dim_elem_it = dimension_elements.begin(); dim_elem_it != dimension_elements.end(); ++dim_elem_it) {
                D dim_elem = *dim_elem_it;
                uint32_t duration = dim_elem.get_duration();

                for (uint32_t local_frame_index = 0; local_frame_index < duration; local_frame_index++, current_frame_index++) {
                    sliced_frames.push_back(DimensionedFrame<D>(dim_elem.get_slice(local_frame_index)));
                }
            }

            return sliced_frames;
        }

    private:
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
     * split    +
     * slice?
     */
    template <DimensionDerived D, DimensionDerived... Ds>
    class HapticEffect : public HapticEffect<D>, public HapticEffect<Ds>... {
    public:
        HapticEffect() = default;

        explicit HapticEffect(std::list<DimensionedFrame<D>> frames, std::list<DimensionedFrame<Ds>>... rest_frames)
                : HapticEffect<D>(frames),
                  HapticEffect<Ds>(rest_frames)...,
                  duration(get_max_dimension_duration<D, Ds...>()) {
            pad_back(duration);

            this->frames = slice_frames<D, Ds...>(
                this->HapticEffect<D>::frames.begin(),
                this->HapticEffect<Ds>::frames.begin()...
            );
        }

        explicit HapticEffect(std::list<D> first_dim, std::list<Ds>... rest_dims)
               : HapticEffect(HapticEffect<D>::slice_frames(first_dim), HapticEffect<Ds>::slice_frames(rest_dims)...) { }

        HapticEffect<D, Ds...> copy() {
            return HapticEffect<D, Ds...>(*this);
        }

        HapticEffect<D, Ds...> scale(double scalar) {
            auto new_effect = this->copy();

            new_effect.HapticEffect<D>::self_scale(scalar);
            (new_effect.HapticEffect<Ds>::self_scale(scalar), ...);

            return new_effect;
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
        HapticEffect<D, Ds..., T, Ts...> then(const HapticEffect<T, Ts...> &other) {
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
        HapticEffect<D, Ds..., T, Ts...> join(const HapticEffect<T, Ts...> &other) {
            return HapticEffect<D, Ds..., T, Ts...>(
                this->HapticEffect<D>::frames,
                this->HapticEffect<Ds>::frames...,
                other.HapticEffect<T>::frames,
                other.HapticEffect<Ts>::frames...
            );
        }

        std::tuple<HapticEffect<D>, HapticEffect<Ds>...> split() {
            return std::make_tuple(this->HapticEffect<D>::copy(), this->HapticEffect<Ds>::copy()...);
        }

        uint32_t get_duration() const override {
            return duration;
        }

        const std::list<DimensionedFrame<D, Ds...>> get_frames() const {
            return frames;
        }

    private:
        std::list<DimensionedFrame<D, Ds...>> frames;
        uint32_t duration;

        template <DimensionDerived T, DimensionDerived... Ts>
        std::list<DimensionedFrame<T, Ts...>> slice_frames(
            typename std::list<DimensionedFrame<T>>::iterator it,
            typename std::list<DimensionedFrame<Ts>>::iterator... its
        ) {
            std::list<DimensionedFrame<T, Ts...>> sliced_frames;

            std::tuple<
                typename std::list<DimensionedFrame<T>>::iterator,
                typename std::list<DimensionedFrame<Ts>>::iterator...
            > iterators = std::make_tuple(it, its...);

            for (int i = 0; i < duration; i++) {
                // Get frame by dereferencing all our iterators and getting slices
                auto get_frame = []<typename Tuple, size_t ... I>(Tuple t, std::index_sequence<I...>) {
                    return DimensionedFrame<D, Ds...>(
                        (*std::get<I>(t)).get_dimension_slice()...
                    );
                };

                auto frame = get_frame(iterators, std::make_index_sequence<1 + sizeof...(Ts)>());

                // Append frame
                sliced_frames.push_back(frame);

                // Increment all iterators
                std::apply([](auto&&... iterators) {
                    ((++iterators), ...);
                }, iterators);
            }

            return sliced_frames;
        }

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
            if constexpr (sizeof...(Ts) == 0)
                // This is the base case where Ts is empty and we just have template arg T
                // Since the condition is a constexpr, the else block is completely discarded at compile-time when it evaluates to true
                // avoiding the base case issue of unpacking empty Ts
                return this->HapticEffect<T>::get_duration();
            else // This else needs to be here explicitly to avoid issues deducing template argument T for base case
                return std::max(
                    // Get max for dimension for type T
                    this->HapticEffect<T>::get_duration(),
                    // Recurse on remaining dimension types Ts
                    get_max_dimension_duration<Ts...>()
                );
        }
    };
}