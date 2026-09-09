#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "IInstance.h"
#include "ArchitectureDescription.h"

using ComponentBuilder = std::function<std::unique_ptr<IInstance>(const InstanceDescription&)>;

class ComponentFactoryRegistry
{
public:
    void registerBuilder(const std::string& componentType, ComponentBuilder builder)
    {
        const auto result = _builders.emplace(componentType, std::move(builder));

        if (!result.second)
        {
            throw std::logic_error("Builder already registered: " + componentType);
        }
    }

    std::unique_ptr<IInstance> create(const InstanceDescription& description) const
    {
        const auto it = _builders.find(description.type);

        if (it == _builders.end())
        {
            throw std::logic_error("No builder for component type: " + description.type);
        }

        return it->second(description);
    }

private:
    std::unordered_map<std::string, ComponentBuilder> _builders;
};
