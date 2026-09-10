#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>


using ComponentBuilder = std::function<std::unique_ptr<IInstance>(const InstanceDescription&)>;


class ComponentBuilderRegistry
{
public:
    void add(const std::string& componentType, const std::string& implementation, ComponentBuilder builder)
    {
        const Key key{ componentType, implementation };

        const auto result = _builders.emplace(key, std::move(builder));

        if (!result.second)
        {
            throw std::logic_error("Builder deja enregistre pour " + componentType + " / " + implementation);
        }
    }

    std::unique_ptr<IInstance> create(const InstanceDescription& description) const
    {
        const Key key{ description.componentType, description.implementation };

        const auto it = _builders.find(key);

        if (it == _builders.end())
        {
            throw std::logic_error("Aucun builder pour " + description.componentType + " / " + description.implementation);
        }

        return it->second(description);
    }

private:
    struct Key
    {
        std::string componentType;
        std::string implementation;

        bool operator==(const Key& other) const
        {
            return (componentType == other.componentType) && (implementation == other.implementation);
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const
        {
            const auto h1 = std::hash<std::string>{}(key.componentType);

            const auto h2 = std::hash<std::string>{}(key.implementation);

            return h1 ^ (h2 << 1);
        }
    };

private:
    std::unordered_map<Key, ComponentBuilder, KeyHash> _builders;
};