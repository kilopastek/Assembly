#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================================
// Registre d'objets non propriétaires
//
// Utilisé pour les containers et les dépendances.
// Les objets enregistrés doivent vivre plus longtemps que les composants.
// ============================================================================

class ObjectRegistry
{
private:
    struct Key
    {
        std::string name;
        std::type_index type;

        bool operator==(const Key& other) const noexcept
        {
            return name == other.name &&
                   type == other.type;
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            const auto nameHash =
                std::hash<std::string>{}(key.name);

            const auto typeHash =
                key.type.hash_code();

            return nameHash ^ (typeHash << 1U);
        }
    };

public:
    template<typename Interface, typename Implementation>
    void bind(
        const std::string& name,
        Implementation& object)
    {
        static_assert(
            std::is_convertible_v<Implementation*, Interface*>,
            "L'objet ne realise pas l'interface demandee"
        );

        const Key key{name, typeid(Interface)};

        const auto [iterator, inserted] =
            objects_.emplace(
                key,
                static_cast<Interface*>(&object)
            );

        if (!inserted)
        {
            throw std::logic_error(
                "Objet deja enregistre : " + name
            );
        }
    }

    template<typename Interface>
    void replace(
        const std::string& name,
        Interface& object)
    {
        const Key key{name, typeid(Interface)};

        objects_.insert_or_assign(
            key,
            static_cast<void*>(&object)
        );
    }

    template<typename Interface>
    Interface& get(const std::string& name) const
    {
        const Key key{name, typeid(Interface)};

        const auto iterator = objects_.find(key);

        if (iterator == objects_.end())
        {
            throw std::logic_error(
                "Objet non enregistre : " + name
            );
        }

        return *static_cast<Interface*>(
            iterator->second
        );
    }

    template<typename Interface>
    bool contains(const std::string& name) const
    {
        return objects_.find(
            Key{name, typeid(Interface)}
        ) != objects_.end();
    }

private:
    std::unordered_map<Key, void*, KeyHash> objects_;
};

using DependencyRegistry = ObjectRegistry;
using ContainerRegistry = ObjectRegistry;