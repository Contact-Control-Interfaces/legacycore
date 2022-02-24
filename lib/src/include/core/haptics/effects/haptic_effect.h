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

#include "core/haptics/frame.h"

template <
        typename Result,
        typename Argument,
        typename Collection,
        std::size_t window_size,
        typename mapper_type
>
static Result map_collection(Collection collection, mapper_type &&mapper);

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

    template <typename A, typename... As>
    class HapticEffect;

    template <typename A>
    class HapticEffect<A> {
    public:
        // Friends with other specializations of HapticEffect<T> so we can access protected `dimension` field
        template <typename, typename...> friend class HapticEffect;

        HapticEffect() = default;
        explicit HapticEffect<A>(std::list<A> atoms) : atoms(atoms.begin(), atoms.end()) { }
        explicit HapticEffect<A>(A atom) : HapticEffect<A>(std::list<A> { atom }) { }

        virtual void pad_back(uint32_t pad_to_length) {
            uint32_t pad_count = std::max((uint32_t)0, (uint32_t)(pad_to_length - atoms.size()));

            for (int i = 0; i < pad_count; i++)
                atoms.push_back(A::get_zero());
        }

        virtual void pad_front(uint32_t pad_to_length) {
            uint32_t pad_count = std::max((uint32_t)0, (uint32_t)(pad_to_length - atoms.size()));

            for (int i = 0; i < pad_count; i++)
                atoms.push_front(A::get_zero());
        }

        HapticEffect<A> copy() const {
            return HapticEffect<A>(*this);
        }

        HapticEffect<A> scale(double scalar) const {
            auto new_effect = this->copy();

            new_effect.self_scale(scalar);

            return new_effect;
        }

        HapticEffect<A> delay(uint32_t length) const {
            auto new_effect = this->copy();

            new_effect.pad_front(length + get_duration());

            return new_effect;
        }

        HapticEffect<A> sleep(uint32_t length) const {
            auto new_effect = this->copy();

            new_effect.pad_back(length + get_duration());

            return new_effect;
        }

        HapticEffect<A> then(A &&other_atom) const {
            auto new_effect = this->copy();

            new_effect.atoms.insert(new_effect.atoms.end(), other_atom);

            return new_effect;
        }

        HapticEffect<A> then(A &other_atom) const {
            auto new_effect = this->copy();

            new_effect.atoms.insert(new_effect.atoms.end(), other_atom);

            return new_effect;
        }

        HapticEffect<A> then(HapticEffect<A> &&other) const {
            auto new_effect = this->copy();

            new_effect.atoms.insert(new_effect.atoms.end(), other.atoms.begin(), other.atoms.end());

            return new_effect;
        }

        HapticEffect<A> then(HapticEffect<A> &other) const {
            auto new_effect = this->copy();

            new_effect.atoms.insert(new_effect.atoms.end(), other.atoms.begin(), other.atoms.end());

            return new_effect;
        }

        template <typename T, typename... Ts>
        HapticEffect<A, T, Ts...> then(HapticEffect<T, Ts...> &other) const {
            auto new_effect = other.copy();

            new_effect.pad_front(get_duration());

            return this->join(new_effect);
        }

        HapticEffect<A> repeat(uint32_t times) const {
            auto new_effect = this->copy();

            if (times == 0)
                return new_effect;

            return new_effect.then(new_effect.repeat(times - 1));
        }

        // TODO see if better way to define same function accepting lvalues and rvalues

        template <typename T, typename... Ts>
        HapticEffect<A, T, Ts...> join(HapticEffect<T, Ts...> &other) const {
            return HapticEffect<A, T, Ts...>(
                this->HapticEffect<A>::atoms,
                other.HapticEffect<T>::atoms,
                other.HapticEffect<Ts>::atoms...
            );
        }

        template <typename T, typename... Ts>
        HapticEffect<A, T, Ts...> join(HapticEffect<T, Ts...> &&other) const {
            return HapticEffect<A, T, Ts...>(
                this->HapticEffect<A>::atoms,
                other.HapticEffect<T>::atoms,
                other.HapticEffect<Ts>::atoms...
            );
        }

        template <std::size_t window_size, typename mapper_type>
        HapticEffect<A> map(mapper_type &&mapper) {
            return map_collection<HapticEffect<A>, A, decltype(atoms), window_size, mapper_type>(atoms, mapper);
        }

        HapticEffect<A> map(std::function<A(A)> &&mapper) {
            return HapticEffect<A>::map<1>(mapper);
        }

//        template <typename Func>
//        HapticEffect<A> interpolate(A to, uint32_t over_frames, Func &&func) {
//            auto new_effect = this-copy();
//
//            auto diff = to - *atoms.end();
//
//            atoms.end() + inc + inc + inc
//
//        }

        virtual uint32_t get_duration() const {
            return atoms.size();
        }

        std::list<A> get_atoms() const {
            return atoms;
        }

        std::list<Frame<A>> get_frames() const {
            std::list<Frame<A>> frames;

            for (typename decltype(atoms)::const_iterator it = atoms.begin(); it != atoms.end(); ++it)
                frames.push_back(Frame<A>(*it));

            return frames;
        }

    protected:
        std::list<A> atoms;

        void self_scale(double scalar) {
            const uint32_t duration = get_duration();
            const auto new_frames_count = static_cast<int32_t>((scalar - 1.0) * duration);

            const uint32_t space_between = duration / abs(new_frames_count);
            const uint32_t initial_offset = space_between / 2;

            auto current_it = atoms.begin();
            std::advance(current_it, initial_offset);

            if (new_frames_count > 0) {
                int i = 0;

                do {
                    atoms.insert(current_it, *current_it);

                    std::advance(current_it, space_between + 1); // +1 to account for newly inserted frame

                    i++;
                } while (i < new_frames_count);
            } else if (new_frames_count < 0) {
                // new_frames_count is negative, so we increment up to 0 in our loop
                int i = new_frames_count;

                do {
                    current_it = atoms.erase(current_it);

                    std::advance(current_it, space_between - 1); // 1 to account for removed frame shrinking the list

                    i++;
                } while (i < 0);
            } // else, new_frames_count is 0 so do nothing;
            // can't use just `else` when `new_frames_count` is negative since we use do-while:
            // the loop condition isn't checked until after the body
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
    template <typename A, typename... As>
    class HapticEffect : public HapticEffect<A>, public HapticEffect<As>... {
    public:
        HapticEffect() = default;

        //TODO move constructors? Move vectors and avoid copying?

        HapticEffect(const HapticEffect<A, As...>& other) : HapticEffect<A, As...>(
            other.HapticEffect<A>::atoms,
            other.HapticEffect<As>::atoms...
        ) { }

        explicit HapticEffect(std::list<A> atoms, std::list<As>... rest_atoms)
                : HapticEffect<A>(atoms),
                  HapticEffect<As>(rest_atoms)...,
                  duration(get_max_dimension_duration<A, As...>()) {
            pad_back(duration);

            this->frames = slice_frames<A, As...>(
                duration,
                this->HapticEffect<A>::atoms.begin(),
                this->HapticEffect<As>::atoms.begin()...
            );
        }

        explicit HapticEffect(A &atom, As&... rest_atoms) : HapticEffect<A, As...>(
            std::list<A> { atom },
            std::list<As> { rest_atoms }...
        ) { }

        HapticEffect<A, As...> copy() const {
            return HapticEffect<A, As...>(*this);
        }

        HapticEffect<A, As...> scale(double scalar) const {
            auto new_effect = this->copy();

            new_effect.HapticEffect<A>::self_scale(scalar);
            (new_effect.HapticEffect<As>::self_scale(scalar), ...);

            return new_effect;
        }

        HapticEffect<A, As...> delay(uint32_t length) const {
            auto new_effect = this->copy();

            new_effect.pad_front(length + get_duration());

            return new_effect;
        }

        HapticEffect<A, As...> sleep(uint32_t length) const {
            auto new_effect = this->copy();

            new_effect.pad_back(length + get_duration());

            return new_effect;
        }

        // TODO rethink this implementation. It does a lot of the same work the constructors do
        HapticEffect<A, As...> then(const HapticEffect<A, As...> &other) const {
            auto new_effect = this->copy();

            new_effect.HapticEffect<A>::atoms.insert(
                new_effect.HapticEffect<A>::atoms.end(),
                other.HapticEffect<A>::atoms.begin(),
                other.HapticEffect<A>::atoms.end()
            );

            (
                new_effect.HapticEffect<As>::atoms.insert(
                    new_effect.HapticEffect<As>::atoms.end(),
                    other.HapticEffect<As>::atoms.begin(),
                    other.HapticEffect<As>::atoms.end()
                ),
                ...
            );

            new_effect.duration = new_effect.template get_max_dimension_duration<A, As...>();

            new_effect.frames = slice_frames<A, As...>(
                new_effect.duration,
                new_effect.HapticEffect<A>::atoms.begin(),
                new_effect.HapticEffect<As>::atoms.begin()...
            );

            return new_effect;
        }

        template <typename T, typename... Ts>
        HapticEffect<A, As..., T, Ts...> then(const HapticEffect<T, Ts...> &other) const {
            auto new_effect = other.copy();

            new_effect.pad_front(get_duration());

            return new_effect.join(*this);
        }

        HapticEffect<A, As...> repeat(uint32_t times) const {
            auto new_effect = this->copy();

            if (times == 0)
                return new_effect;

            return new_effect.then(new_effect.repeat(times - 1));
        }

        template <typename T, typename... Ts>
        HapticEffect<A, As..., T, Ts...> join(const HapticEffect<T, Ts...> &other) const {
            return HapticEffect<A, As..., T, Ts...>(
                this->HapticEffect<A>::atoms,
                this->HapticEffect<As>::atoms...,
                other.HapticEffect<T>::atoms,
                other.HapticEffect<Ts>::atoms...
            );
        }

        template <std::size_t window_size, typename mapper_type>
        HapticEffect<A, As...> map(mapper_type &&mapper) {
            return map_collection<
                HapticEffect<A, As...>, Frame<A, As...>, decltype(frames), window_size, mapper_type
            >(frames, mapper);
        }

        HapticEffect<A, As...> map(std::function<Frame<A, As...>(Frame<A, As...>)> &&mapper) {
            return HapticEffect<A, As...>::map<1>(mapper);
        }

        std::tuple<HapticEffect<A>, HapticEffect<As>...> split() const {
            return std::make_tuple(this->HapticEffect<A>::copy(), this->HapticEffect<As>::copy()...);
        }

        uint32_t get_duration() const override {
            return duration;
        }

        const std::list<Frame<A, As...>> get_frames() const {
            return frames;
        }

    protected:
        void pad_back(uint32_t pad_to_length) override {
            HapticEffect<A>::pad_back(pad_to_length);
            (HapticEffect<As>::pad_back(pad_to_length), ...);

            duration = pad_to_length;

            this->frames = slice_frames<A, As...>(
                duration,
                this->HapticEffect<A>::atoms.begin(),
                this->HapticEffect<As>::atoms.begin()...
            );
        }

        void pad_front(uint32_t pad_to_length) override {
            HapticEffect<A>::pad_front(pad_to_length);
            (HapticEffect<As>::pad_front(pad_to_length), ...);

            duration = pad_to_length;

            this->frames = slice_frames<A, As...>(
                duration,
                this->HapticEffect<A>::atoms.begin(),
                this->HapticEffect<As>::atoms.begin()...
            );
        }

    private:
        std::list<Frame<A, As...>> frames;
        uint32_t duration;

        /*
         * Assumes the lists are all equal length (dimensions have already been padded)
         */
        template <typename T, typename... Ts>
        std::list<Frame<T, Ts...>> slice_frames(
            std::size_t length,
            typename std::list<T>::iterator it,
            typename std::list<Ts>::iterator... its
        ) const {
            std::list<Frame<T, Ts...>> sliced_frames;

            std::tuple<
                typename std::list<T>::iterator,
                typename std::list<Ts>::iterator...
            > iterators = std::make_tuple(it, its...);

            for (int i = 0; i < length; i++) {
                // Get frame by dereferencing all our iterators and getting slices
                auto get_frame = []<typename Tuple, size_t ... I>(Tuple t, std::index_sequence<I...>) {
                    return Frame<A, As...>(
                        (*std::get<I>(t))...
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

        template<typename T, typename... Ts>
        uint32_t get_max_dimension_duration() const {
            return std::max({
                this->HapticEffect<T>::get_duration(),
                this->HapticEffect<Ts>::get_duration()...
            });
        }
    };
}

template <
    typename Result,
    typename Argument,
    typename Collection,
    std::size_t window_size,
    typename mapper_type
>
static Result map_collection(Collection collection, mapper_type &&mapper) {
    static_assert(window_size > 0, "window_size for map must be at least 1");

    Collection new_frames;

    for (typename decltype(collection)::iterator it = collection.begin(); it != collection.end(); ++it) {
        if constexpr (window_size == 1) {
            new_frames.push_back(mapper(*it));
        } else {
            std::array<Argument, window_size / 2> behind;
            std::array<Argument, window_size / 2> ahead;

            const auto access_or_zero = [collection](typename decltype(collection)::iterator it) {
                return (it == collection.rend() || it == collection.end()) ? Argument::get_zero() : *it;
            };

            for (int j = 0; j < window_size / 2; j++) {
                behind[j] = access_or_zero(std::ranges::prev(it, j, collection.rend()));
                ahead[j] = access_or_zero(std::ranges::prev(it, j, collection.end()));
            }

            new_frames.push_back(mapper(behind, *it, ahead));
        }
    }

    return Result(new_frames);
}