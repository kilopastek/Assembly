#pragma once

#include <any>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace bdd
{
    class ScenarioContext
    {
    public:
        template<typename T, typename... Args>
        T& getOrCreateState(Args&&... args)
        {
            const std::type_index key{typeid(T)};

            auto it = _states.find(key);

            if (it == _states.end())
            {
                it = _states.emplace(key, std::any{ std::in_place_type<T>, std::forward<Args>(args)... }).first;
            }

            return std::any_cast<T&>(it->second);
        }

        template<typename T>
        T& requireState()
        {
            const auto it = _states.find(std::type_index{typeid(T)});

            if (it == _states.end())
            {
                throw std::logic_error("Etat absent du ScenarioContext");
            }

            return std::any_cast<T&>(it->second);
        }

        template<typename T>
        const T& requireState() const
        {
            const auto it = _states.find(std::type_index{typeid(T)});

            if (it == _states.end())
            {
                throw std::logic_error("Etat absent du ScenarioContext");
            }

            return std::any_cast<const T&>(it->second);
        }

        template<typename T>
        bool hasState() const
        {
            return _states.find(std::type_index{ typeid(T) }) != _states.end();
        }

        void clear()
        {
            _states.clear();
        }

    private:
        std::unordered_map<std::type_index, std::any> _states;
    };
}
