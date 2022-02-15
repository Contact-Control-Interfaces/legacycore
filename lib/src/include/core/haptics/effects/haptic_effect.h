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

    class HapticEffect {
    public:
        static ZeroHapticEffect &ZERO;

        virtual Frame &get_current_frame() = 0;
        virtual void move_next_frame() = 0;

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

        Frame &get_current_frame() override;
        void move_next_frame() override;

        HapticEffectSequence chain(HapticEffect &other) override;
        ZeroHapticEffect scale(double scalar);
    private:
        NullFrame null_frame;
        uint32_t duration = 1;
    };

    // Can't use concept constraints with CRTP due to incomplete types; use static_assert in base constructor instead
    template <typename T>
    class TypedHapticEffect : public HapticEffect {
    public:
        virtual T scale(double scalar) = 0;
    protected:
        explicit TypedHapticEffect(double scalar = 1.0);
    };

    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequence> {
    public:
//        class Iterator {
//        public:
//            using iterator_category = std::forward_iterator_tag;
//            using difference_type   = std::ptrdiff_t;
//            using value_type = HapticEffect;
//            using pointer = std::reference_wrapper<value_type>;
//            using reference = value_type&;
//
//            bool eod = false;
//
//            // Sets up an Iterator, starting at the beginning
//            explicit Iterator(HapticEffectSequence& sequence)
//                : Iterator(sequence, sequence.current_effect){}
//
//            // Sets up an Iterator, setting the start effect manually
//            Iterator(HapticEffectSequence& sequence, pointer effect)
//                : pHapticEffect(effect), sequence(sequence) { }
//
//            reference operator*() const { return pHapticEffect.get(); }
//            pointer operator->() { return pHapticEffect; }
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
//                return a.sequence == b.sequence
//                    && (a.pHapticEffect == b.pHapticEffect || (a.eod && b.eod));
//            };
//
//            friend bool operator!= (const Iterator& a, const Iterator& b) {
//                return !(a == b);
//            };
////
////            friend Iterator operator+(const Iterator &other, int n) {
////
////            }
//
//        private:
//            pointer pHapticEffect;
//            HapticEffectSequence& sequence;
//
//            void increment() {
//                sequence.move_next_frame();
//                if (true /* TODO sequence not at EOD */){
//                    pHapticEffect = sequence.current_effect;
//                } else {
//                   // pHapticEffect = HapticEffect::NOTHING;
//                    eod = true;
//                }
//            }
//        };

        explicit HapticEffectSequence(HapticEffect &initial);

        HapticEffectSequence(const HapticEffectSequence &other);

        HapticEffectSequence scale(double scalar) override;

        HapticEffectSequence chain(HapticEffect &other) override;

        Frame &get_current_frame() override;

        void move_next_frame() override;

        uint32_t get_duration() const override;

        //Iterator begin() { return Iterator(*this); }
        //Iterator end() { return Iterator::NIL; }

    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
        HapticEffect &current_effect;
    };

    template <typename T>
    TypedHapticEffect<T>::TypedHapticEffect(double scalar) : HapticEffect(scalar) {
        static_assert(std::is_base_of<HapticEffect, T>::value);
    }

//    void test_scale(){
//        std::cout << "Creating sequence" << std::endl;
//        HapticEffectSequence sequence(HapticEffect::ZERO);
//        std::cout << sequence.get_duration() << std::endl;
//        sequence.scale(2);
//        std::cout << sequence.get_duration() << std::endl;
//    }

    /*HapticEffect &&ZeroHapticEffect::stretch(uint32_t new_duration){
        ZeroHapticEffect new_zero;

        new_zero.duration = new_duration;

        return std::move(new_zero); // what
    }*/
}
