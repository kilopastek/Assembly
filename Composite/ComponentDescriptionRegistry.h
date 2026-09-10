#pragma once
#include <string>
#include <unordered_map>
#include "ComponentPathResolver.h"
#include "ComponentDescriptionLoader.h"

class ComponentDescriptionRegistry
{
public:
    ComponentDescriptionRegistry(const ComponentPathResolver& paths, const ComponentDescriptionLoader& loader)
        : _paths(paths)
        , _loader(loader)
    {
    }

    const ComponentDescription& getModule(const std::string& componentType)
    {
        const auto it = _descriptions.find(componentType);

        if (it != _descriptions.end())
        {
            return it->second;
        }

        ComponentDescription description = _loader.load(_paths.moduleComponentFile(componentType));

        const auto result = _descriptions.emplace(componentType, std::move(description));

        return result.first->second;
    }

private:
    const ComponentPathResolver& _paths;
    const ComponentDescriptionLoader& _loader;

    std::unordered_map<std::string, ComponentDescription> _descriptions;
};
