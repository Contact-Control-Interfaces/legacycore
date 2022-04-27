//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "../frame.h"
#include "effect.h"

#include <memory>
#include <list>
#include <iostream>
#include <comms/communicator.h>

namespace contactci::core::haptics {

    // TODO enforce that S is a subclass of Atom
    template<typename A>
    class AtomPlayer {
    public:
        static void play(contactci::comms::Communicator &comms, A atom);
    };

    template <typename A, typename... As>
    class FramePlayer {
    public:
        static void play(contactci::comms::Communicator &comms, const Frame<A, As...> &frame);
    };

    template <typename A, typename... As>
    class PlayingEffect {
    public:
        using FrameIterator = typename std::list<Frame<A, As...>>::const_iterator;

        // TODO should we also pass some additional state so callback can distinguish between playing ended/aborted
        // vs effect playing until end
        ~PlayingEffect() {
            if (on_effect_completed)
                on_effect_completed(&effect);
        }

        PlayingEffect(const PlayingEffect<A, As...> &other)
                : effect(other.effect), begin(other.begin), current(other.current), end(other.end),
                  on_effect_completed(other.on_effect_completed) { }

        PlayingEffect(PlayingEffect<A, As...> &&other)
                : effect(other.effect), begin(other.begin), current(other.current), end(other.end),
                  on_effect_completed(other.on_effect_completed) { }

        PlayingEffect(
            effects::Effect<A, As...> &effect,
            void (*on_effect_completed)(effects::Effect<A, As...>*) = nullptr
        ) : PlayingEffect(effect, effect.get_frames(), on_effect_completed)
        { }

        PlayingEffect<A, As...> &operator=(const PlayingEffect<A, As...> &other) {
            PlayingEffect<A, As...> temp(other);
            std::swap(other, *this);
            return *this;
        }

        PlayingEffect<A, As...> &operator=(PlayingEffect<A, As...> &&other) {
            std::swap(other, *this); //Don't care what's left in `other`, no copy needed
            return *this;
        }

        inline bool is_at_end() {
            return current == end;
        }

        Frame<A, As...> get_current_and_increment() {
            return *current++;
        }

    private:
        PlayingEffect(
            effects::Effect<A, As...> &effect,
            std::list<Frame<A, As...>> frames,
            void (*on_effect_completed)(effects::Effect<A, As...>*)
        ) : effect(effect),
            begin(frames.begin()),
            current(begin),
            end(frames.end()),
            on_effect_completed(on_effect_completed)
        { }

        effects::Effect<A, As...> &effect;

        void (*on_effect_completed)(effects::Effect<A, As...>*);

        FrameIterator begin;
        FrameIterator current;
        FrameIterator end;
    };


    template <typename A, typename ...As>
    void FramePlayer<A, As...>::play(contactci::comms::Communicator &comms, const Frame<A, As...> &frame) {
        comms.start_frame();

        play_frame<A, As...>(comms, frame);

        comms.end_frame();
    }

    template <typename A, typename ...As>
    static void play_frame(contactci::comms::Communicator &comms, const Frame<A, As...> &frame) {
        AtomPlayer<A>::play(comms, frame.Frame<A>::get_atom());
        (AtomPlayer<As>::play(comms, frame.Frame<As>::get_atom()), ...);
    }
}
