//
// Created by john_contactci on 3/10/2022.
//

#pragma once

#include <functional>
#include <list>

template <
    typename Argument,
    typename Collection,
    std::size_t window_size,
    typename mapper_type
>
static inline void map_collection(Collection &collection, mapper_type &&mapper);

namespace contactci::core::haptics::effects {

    template <typename A, typename... As>
    class EffectBuilder;

    template <typename A>
    class EffectBuilder<A> {
    public:
        template<typename, typename...> friend class EffectBuilder;

        EffectBuilder() = default;
        explicit EffectBuilder<A>(const std::list<A> &atoms) : atoms(atoms.begin(), atoms.end()) { }
        explicit EffectBuilder<A>(const std::list<A> &&atoms) : atoms(atoms) { }
        explicit EffectBuilder<A>(A atom) : EffectBuilder<A>(std::list<A> { atom }) { }

        EffectBuilder<A> &scale(double scalar) {
            const uint32_t duration = atoms.size();
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
            return *this;
        }

        EffectBuilder<A> &delay(uint32_t length) {
            pad_front(length + atoms.size());

            return *this;
        }

        EffectBuilder<A> &sleep(uint32_t length) {
            pad_back(length + atoms.size());

            return *this;
        }

        EffectBuilder<A> &then(A &&other_atom) {
            atoms.push_back(std::move(other_atom));

            return *this;
        }

        EffectBuilder<A> &then(A &other_atom) {
            atoms.push_back(other_atom);

            return *this;
        }

        EffectBuilder<A> &then(Effect<A> &&other) {
            atoms.insert(atoms.end(), other.atoms.begin(), other.atoms.end());

            return *this;
        }

        EffectBuilder<A> &then(Effect<A> &other) {
            atoms.insert(atoms.end(), other.atoms.begin(), other.atoms.end());

            return *this;
        }

        template <typename T, typename... Ts>
        EffectBuilder<A, T, Ts...> then(Effect<T, Ts...> &other) {
            auto other_copy = other.copy();

            // TODO figure out padding other
            other_copy->pad_front(atoms.size());

            return this->join(other_copy);
        }

        template <std::size_t window_size, typename mapper_type>
        EffectBuilder<A> &map(mapper_type &&mapper) {
            map_collection<A, decltype(atoms), window_size, mapper_type>(atoms, mapper);

            return *this;
        }

        EffectBuilder<A> &map(std::function<A(A)> &&mapper) {
            return EffectBuilder<A>::map<1>(mapper);
        }

        EffectBuilder<A> &repeat(uint32_t times) {
            int original_size = std::distance(atoms.begin(), atoms.end());

            for (int i = 0; i < times; i++)
                atoms.insert(atoms.end(), atoms.begin(), std::next(atoms.begin(), original_size));

            return *this;
        }

        template <typename T, typename... Ts>
        EffectBuilder<A, T, Ts...> join(Effect<T, Ts...> &other) {
            return EffectBuilder<A, T, Ts...>(
                this->EffectBuilder<A>::atoms,
                other.Effect<T>::atoms,
                other.Effect<Ts>::atoms...
            );
        }

        template <typename T, typename... Ts>
        EffectBuilder<A, T, Ts...> join(Effect<T, Ts...> &&other) {
            return EffectBuilder<A, T, Ts...>(
                this->EffectBuilder<A>::atoms,
                other.Effect<T>::atoms,
                other.Effect<Ts>::atoms...
            );
        }

        template <typename DistortFunc>
        EffectBuilder<A> &interpolate(A to, uint32_t over_frames, DistortFunc &&distort) {
            float step = 1.0f / (float)over_frames;

            for (int i = 0; i < over_frames; i++) {
                float t = step * (float)(i + 1);
                A lerped = this->atoms.back().lerp(to, distort(t));

                atoms.push_back(lerped);
            }

            return *this;
        }

        template <typename DistortFunc>
        EffectBuilder<A> &dampen(uint32_t over_frames, DistortFunc &&distort) {
            return interpolate(A::get_zero(), over_frames, distort);
        }

        // TODO move or copy atoms? Move invalidates this object
        Effect<A> build() {
            return Effect<A>(std::move(this->atoms));
        }

    protected:
        std::list<A> atoms;

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
    };

    template <typename A, typename... As>
    class EffectBuilder : public EffectBuilder<A>, public EffectBuilder<As>... {
    public:
        EffectBuilder() = default;

        explicit EffectBuilder(const Effect<A, As...>& other) : EffectBuilder<A, As...>(
            other.Effect<A>::atoms,
            other.Effect<As>::atoms...
        ) { }

        explicit EffectBuilder(const std::list<A> &atoms, const std::list<As>&... rest_atoms)
                : EffectBuilder<A>(atoms),
                  EffectBuilder<As>(rest_atoms)... {
            pad_back(get_duration());
        }

        explicit EffectBuilder(const std::list<A> &&atoms, const std::list<As>&&... rest_atoms)
                : EffectBuilder<A>(std::move(atoms)),
                  EffectBuilder<As>(std::move(rest_atoms))... {
            pad_back(get_duration());
        }

        explicit EffectBuilder(A &atom, As&... rest_atoms) : EffectBuilder<A, As...>(
            std::list<A> { atom },
            std::list<As> { rest_atoms }...
        ) { }

        EffectBuilder<A, As...> &scale(double scalar) {
            EffectBuilder<A>::scale(scalar);
            (EffectBuilder<As>::scale(scalar), ...);

            return *this;
        }

        EffectBuilder<A, As...> &delay(uint32_t length) {
            pad_front(get_duration() + length);

            return *this;
        }

        EffectBuilder<A, As...> &sleep(uint32_t length) {
            pad_back(get_duration() + length);

            return *this;
        }

        EffectBuilder<A, As...> &then(const Effect<A, As...> &other) {
            EffectBuilder<A>::atoms.insert(
                EffectBuilder<A>::atoms.end(),
                other.Effect<A>::atoms.begin(),
                other.Effect<A>::atoms.end()
            );

            (
                EffectBuilder<As>::atoms.insert(
                    EffectBuilder<As>::atoms.end(),
                    other.Effect<As>::atoms.begin(),
                    other.Effect<As>::atoms.end()
                ),
                ...
            );

            return *this;
        }

        template <typename T, typename... Ts>
        EffectBuilder<A, As..., T, Ts...> then(const Effect<T, Ts...> &other) const {
            auto new_effect = other.copy();

            new_effect->pad_front(get_duration());

            return new_effect->join(*this);
        }

        //TODO figure out map implementation
        // We don't have a collection since we dont slice frames until `build()`
//        template <std::size_t window_size, typename mapper_type>
//        EffectBuilder<A, As...> &map(mapper_type &&mapper) const {
//            return map_collection<Frame<A, As...>, decltype(frames), window_size, mapper_type
//            >(frames, mapper);
//        }
//
//        EffectBuilder<A, As...> &map(std::function<Frame<A, As...>(Frame<A, As...>)> &&mapper) const {
//            return Effect<A, As...>::map<1>(mapper);
//        }

        EffectBuilder<A, As...> &repeat(uint32_t times) {
            EffectBuilder<A>::repeat(times);
            (EffectBuilder<As>::repeat(times), ...);

            return *this;
        }

        template <typename T, typename... Ts>
        EffectBuilder<A, As..., T, Ts...> join(const Effect<T, Ts...> &other) const {
            return EffectBuilder<A, As..., T, Ts...>(
                EffectBuilder<A>::atoms,
                EffectBuilder<As>::atoms...,
                other.Effect<T>::atoms,
                other.Effect<Ts>::atoms...
            );
        }

        template <typename DistortFunc>
        EffectBuilder<A, As...> &interpolate(Frame<A, As...> &to, uint32_t over_frames, DistortFunc &&distort) {
            EffectBuilder<A>::interpolate(to.template get_atom<A>(), over_frames, distort);
            (EffectBuilder<As>::interpolate(to.template get_atom<As>(), over_frames, distort), ...);

            return *this;
        }

        template <typename DistortFunc>
        EffectBuilder<A, As...> &interpolate(Frame<A, As...> &&to, uint32_t over_frames, DistortFunc &&distort) {
            EffectBuilder<A>::interpolate(to.template get_atom<A>(), over_frames, distort);
            (EffectBuilder<As>::interpolate(to.template get_atom<As>(), over_frames, distort), ...);

            return *this;
        }

        template <typename DistortFunc>
        EffectBuilder<A, As...> &dampen(uint32_t over_frames, DistortFunc &&distort) {
            return interpolate(Frame<A, As...>::get_zero(), over_frames, distort);
        }

        // TODO move or copy atoms? Move invalidates this object
        Effect<A, As...> build() {
            return Effect<A, As...>(
                std::move(EffectBuilder<A>::atoms),
                std::move(EffectBuilder<As>::atoms)...
            );
        }

        uint32_t get_duration() const {
            return get_max_dimension_duration<A, As...>();
        }

    protected:
        void pad_back(uint32_t pad_to_length) override {
            EffectBuilder<A>::pad_back(pad_to_length);
            (EffectBuilder<As>::pad_back(pad_to_length), ...);
        }

        void pad_front(uint32_t pad_to_length) override {
            EffectBuilder<A>::pad_front(pad_to_length);
            (EffectBuilder<As>::pad_front(pad_to_length), ...);
        }

    private:
        template<typename T, typename... Ts>
        uint32_t get_max_dimension_duration() const {
            return std::max({
                EffectBuilder<T>::atoms.size(),
                EffectBuilder<Ts>::atoms.size()...
            });
        }
    };
}

template <
    typename Argument,
    typename Collection,
    std::size_t window_size,
    typename mapper_type
>
static inline void map_collection(Collection &collection, mapper_type &&mapper) {
    static_assert(window_size > 0, "window_size for map must be at least 1");

    for (auto it = collection.begin(); it != collection.end(); ++it) {
        auto current = *it;

        if constexpr (window_size == 1) {
            *it = mapper(current);
        } else {
            std::array<Argument, window_size / 2> behind;
            std::array<Argument, window_size / 2> ahead;

            const auto access_or_zero = [collection](auto it) {
                return (it == collection.rend() || it == collection.end()) ? Argument::get_zero() : *it;
            };

            for (int j = 0; j < window_size / 2; j++) {
                behind[j] = access_or_zero(std::max(it - j, collection.rend()));
                ahead[j] = access_or_zero(std::min(it + j, collection.end()));
            }

            *it = mapper(behind, current, ahead);
        }
    }
}