//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include "core/haptics/effects/haptic_effect.h"
#include <functional>

#include <vector>

namespace contactci::core::haptics::effects {


    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequence> {
    public:
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type = HapticEffect;
            using pointer = std::reference_wrapper<value_type>;
            using reference = value_type&;

            bool eod = false;

            explicit Iterator(HapticEffectSequence& sequence)
                : Iterator(sequence, sequence.current_effect){}

                // Sets up an Iterator, starting
            Iterator(HapticEffectSequence& sequence, pointer effect)
                : pHapticEffect(effect), sequence(sequence) { }

            reference operator*() const { return pHapticEffect.get(); }
            pointer operator->() { return pHapticEffect; }

            // Prefix increment
            Iterator& operator++() {
                increment();
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                increment();
                return tmp;
            }

            friend bool operator== (const Iterator& a, const Iterator& b) {
                return a.sequence == b.sequence
                    && (a.pHapticEffect == b.pHapticEffect || a.eod == b.eod);
            };

            friend bool operator!= (const Iterator& a, const Iterator& b) {
                return !(a == b);
            };

        private:
            pointer pHapticEffect;
            HapticEffectSequence& sequence;

            void increment(){
                sequence.move_next_frame();
                if (true /* TODO sequence not at EOD */){
                    pHapticEffect = sequence.current_effect;
                } else {
                    pHapticEffect = HapticEffect::NOTHING;
                    eod = true;
                }
            }
        };

        explicit HapticEffectSequence(HapticEffect &initial);

        HapticEffectSequence(const HapticEffectSequence &other);

        HapticEffectSequence scale(double scalar) override;

        HapticEffectSequence chain(HapticEffect &other) override;

        Frame &get_current_frame() override;

        void move_next_frame() override;

        uint32_t get_duration() const override;

        Iterator begin() { return Iterator(*this); }
        Iterator end() { return Iterator::NIL; }

    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
        HapticEffect &current_effect;
    };
}