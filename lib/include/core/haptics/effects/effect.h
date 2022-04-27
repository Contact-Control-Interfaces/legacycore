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
#include <memory>

#include "../frame.h"
#include "interpolation.h"

namespace contactci::core::haptics::effects {

    // https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

    template <typename A, typename... As>
    class Effect;

    template <typename A>
    class Effect<A> {
    public:
        // Friends with other specializations of Effect<T> so we can access protected `dimension` field
        template <typename, typename...> friend class EffectBuilder;

        Effect() : atoms() { };
        explicit Effect<A>(std::list<A> &atoms) : atoms(atoms.begin(), atoms.end()) { }
        explicit Effect<A>(std::list<A> &&atoms) : atoms(atoms) { }
        explicit Effect<A>(A atom) : Effect<A>(std::list<A> { atom }) { }

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

        Effect<A> copy() const {
            return Effect<A>(*this);
        }

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
    };

    template <typename A, typename... As>
    class Effect : public Effect<A>, public Effect<As>... {
    public:
        template <typename, typename...> friend class EffectBuilder;

        Effect() : Effect<A>(), Effect<As>()... { };

        //TODO move constructors? Move vectors and avoid copying?

        Effect(const Effect<A, As...>& other) : Effect<A, As...>(
            other.Effect<A>::atoms,
            other.Effect<As>::atoms...
        ) { }

        explicit Effect(std::list<A> atoms, std::list<As>... rest_atoms)
                : Effect<A>(atoms),
                  Effect<As>(rest_atoms)...,
                  duration(get_max_dimension_duration<A, As...>()) {
            pad_back(duration);

            this->frames.clear();

            slice_frames<A, As...>(
                this->frames,
                duration,
                this->Effect<A>::atoms.begin(),
                this->Effect<As>::atoms.begin()...
            );
        }

        explicit Effect(A &atom, As&... rest_atoms) : Effect<A, As...>(
            std::list<A> { atom },
            std::list<As> { rest_atoms }...
        ) { }

        Effect<A, As...> copy() const {
            return Effect<A, As...>(*this);
        }

        uint32_t get_duration() const override {
            return duration;
        }

        const std::list<Frame<A, As...>> get_frames() const {
            return frames;
        }

    protected:
        void pad_back(uint32_t pad_to_length) override {
            Effect<A>::pad_back(pad_to_length);
            (Effect<As>::pad_back(pad_to_length), ...);

            duration = pad_to_length;

            this->frames.clear();

            slice_frames<A, As...>(
                this->frames,
                duration,
                this->Effect<A>::atoms.begin(),
                this->Effect<As>::atoms.begin()...
            );
        }

        void pad_front(uint32_t pad_to_length) override {
            Effect<A>::pad_front(pad_to_length);
            (Effect<As>::pad_front(pad_to_length), ...);

            duration = pad_to_length;

            this->frames.clear();

            slice_frames<A, As...>(
                this->frames,
                duration,
                this->Effect<A>::atoms.begin(),
                this->Effect<As>::atoms.begin()...
            );
        }

    private:
        std::list<Frame<A, As...>> frames;
        uint32_t duration;

        /*
         * Assumes the lists are all equal length (dimensions have already been padded)
         */
        // TODO pull out as static inline function rather than instance method
        // TODO should this logic be part of EffectBuilder?
        template <typename T, typename... Ts>
        void slice_frames(
            std::list<Frame<T, Ts...>> &sliced_frames,
            std::size_t length,
            typename std::list<T>::iterator it,
            typename std::list<Ts>::iterator... its
        ) const {
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
        }

        template<typename T, typename... Ts>
        uint32_t get_max_dimension_duration() const {
            return std::max({
                this->Effect<T>::get_duration(),
                this->Effect<Ts>::get_duration()...
            });
        }
    };
}

