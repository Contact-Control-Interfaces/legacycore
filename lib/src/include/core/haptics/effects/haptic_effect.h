//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <vector>

#include "core/haptics/frame.h"

namespace contactci::core::haptics::effects {

    class ZeroHapticEffect;
    class HapticEffectSequence;

    class FrameIterator {

    };

    template <typename T>
    concept FrameIteratorDerived = std::is_base_of<FrameIterator, T>::value;

    class ZeroFrameIterator : FrameIterator {

    };

    // TODO Sequence iterator of effects
    // Effect iterator of frames
    // Player gets an effect, iterates across the frames, calls the template specialized overlaod of play function
    // for the dimension slices of the frame

    // TODO HapticEffect is a container of frames
    // Needs to return a collection of Frames (iterator?)
    class HapticEffect {
    public:
        static ZeroHapticEffect &ZERO;

        virtual FrameIterator get_frames() const = 0;

        virtual uint32_t get_duration() const;
        virtual double get_scalar() const;

        virtual HapticEffectSequence chain(HapticEffect &other) = 0;

        ~HapticEffect() = default;

        friend bool operator== (const HapticEffect& a, const HapticEffect& b) {
            return a.scalar == b.scalar;
                // TODO actually compare effects
        };

        friend bool operator!= (const HapticEffect& a, const HapticEffect& b) {
            return !(a == b);
        };

    protected:
        explicit HapticEffect(uint32_t duration, double scalar = 1.0);

        uint32_t duration;
        double scalar;
    };

    class ZeroHapticEffect : public HapticEffect {
    public:
        ZeroHapticEffect();
        ZeroHapticEffect(const ZeroHapticEffect &other);
        ZeroHapticEffect(uint32_t duration);

        FrameIterator get_frames() const override;

        HapticEffectSequence chain(HapticEffect &other) override;
        ZeroHapticEffect scale(double scalar);
    private:
        NullFrame null_frame;
        uint32_t duration = 1;
    };

    // Can't use concept constraints with CRTP due to incomplete types; use static_assert in base constructor instead
    template <FrameIteratorDerived Iterator, typename T>
    class TypedHapticEffect : public HapticEffect {
    public:
//        class Iterator {
//        public:
//            using iterator_category = std::forward_iterator_tag;
//            using difference_type   = std::ptrdiff_t;
//            using value_type = Frame;
//            using pointer = Frame*;
//            using reference = Frame&;
//
//            // Prefix increment
//            Iterator& operator++() {
//                increment();
//                return *this;
//            }
//
//            // Postfix increment
//            Iterator operator++(int) {
//                Iterator tmp = *this;
//                increment();
//                return tmp;
//            }
//
//            friend bool operator== (const Iterator& a, const Iterator& b) {
//                return a.pFrame == b.pFrame || (a.eod && b.eod);
//            };
//
//            friend bool operator!= (const Iterator& a, const Iterator& b) {
//                return !(a == b);
//            };
//
//        private:
//            pointer pFrame;
//
//            void increment() {
//                this->move_next_frame();
//
//            }
//        };
        Iterator get_frames() const override = 0;

        virtual T scale(double scalar) = 0;
    protected:
        explicit TypedHapticEffect(double scalar = 1.0);
    };

    class HapticEffectSequenceIterator : FrameIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type = HapticEffect;
        using pointer = HapticEffect*;
        using reference = HapticEffect&;

        bool eod = false;

        // Sets up an Iterator, starting at the beginning
        explicit HapticEffectSequenceIterator(HapticEffectSequence& sequence)
                : HapticEffectSequenceIterator(sequence, &sequence.current_effect){}

        // Sets up an Iterator, setting the start effect manually
        HapticEffectSequenceIterator(HapticEffectSequence& sequence, pointer effect)
                : pHapticEffect(effect), sequence(sequence) { }

        reference operator*() const { return *pHapticEffect; }
        pointer operator->() { return pHapticEffect; }

        // Prefix increment
        HapticEffectSequenceIterator& operator++() {
            increment();
            return *this;
        }

        // Postfix increment
        HapticEffectSequenceIterator operator++(int) {
            HapticEffectSequenceIterator tmp = *this;
            increment();
            return tmp;
        }

        friend bool operator== (const HapticEffectSequenceIterator& a, const HapticEffectSequenceIterator& b) {
            return a.sequence == b.sequence
                   && (a.pHapticEffect == b.pHapticEffect || (a.eod && b.eod));
        };

        friend bool operator!= (const HapticEffectSequenceIterator& a, const HapticEffectSequenceIterator& b) {
            return !(a == b);
        };

    private:
        pointer pHapticEffect;
        HapticEffectSequence& sequence;

        void increment() {
//            sequence.move_next_frame();
//            if (!eod){
//                pHapticEffect = &sequence.current_effect;
//            } else {
//                pHapticEffect = &HapticEffect::ZERO;
//                eod = true;
//            }
        }
    };

    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequenceIterator, HapticEffectSequence> {
    public:
        explicit HapticEffectSequence(HapticEffect &initial);

        HapticEffectSequence(const HapticEffectSequence &other);

        HapticEffectSequence scale(double scalar) override;

        HapticEffectSequence chain(HapticEffect &other) override;

        HapticEffectSequenceIterator get_frames() const override;

        //Iterator begin() { return Iterator(*this); }
        //Iterator end() { return Iterator(); }

    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
        HapticEffect &current_effect;
    };

    template <FrameIteratorDerived Iterator, typename T>
    TypedHapticEffect<Iterator, T>::TypedHapticEffect(double scalar) : HapticEffect(scalar) {
        static_assert(std::is_base_of<HapticEffect, T>::value);
    }
}
