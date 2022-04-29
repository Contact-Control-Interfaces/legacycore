//
// Created by john_contactci on 2/2/2022.
//

#pragma once

namespace contactci::core::haptics {

    // https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

    template <typename A>
    class Atom  {
    public:
        static A get_zero();

        virtual A lerp(A to, double t) const = 0;

        virtual bool equals(A other) const = 0;

        bool operator==(A other) const {
            return equals(other);
        }

        bool operator!=(A other) const {
            return !equals(other);
        }
    };

    template <typename A, typename... As>
    class Frame;

    template <typename A>
    class Frame<A> {
    public:
        static Frame<A> get_zero();

        explicit Frame<A>(A atom) : atom(atom) { }

        virtual A get_atom() const {
            return atom;
        }

        virtual void set_atom(const A atom) {
            this->atom = atom;
        }

    private:
        A atom;
    };

    template <typename A, typename... As>
    class Frame : public Frame<A>, public Frame<As>... {
    public:
        static Frame<A, As...> get_zero();

        explicit Frame(A atom, As... rest_atom)
            : Frame<A>(atom), Frame<As>(rest_atom)... { }

        template <typename T>
        T get_atom() const {
            return this->Frame<T>::get_atom();
        }

        template <typename T>
        void set_atom(const T slice) {
            this->Frame<T>::set_atom(slice);
        }
    };

    template <typename A>
    Frame<A> Frame<A>::get_zero() {
        return Frame<A>(A::get_zero());
    }

    template <typename A, typename... As>
    Frame<A, As...> Frame<A, As...>::get_zero() {
        return Frame<A, As...>(A::get_zero(), As::get_zero()...);
    }
}
