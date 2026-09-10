#pragma once
#include <string>
#include <unordered_map>
#include "AssemblyPathResolver.h"
#include "AssemblyDescriptionLoader.h"

class AssemblyDescriptionRegistry
{
public:
    AssemblyDescriptionRegistry(const ComponentPathResolver& paths, const AssemblyDescriptionLoader& loader)
        : _paths(paths)
        , _loader(loader)
    {
    }

    const AssemblyDescription& getModule(const std::string& componentType)
    {
        const auto it = _assemblies.find(componentType);

        if (it != _assemblies.end())
        {
            return it->second;
        }

        AssemblyDescription description = _loader.load(_paths.moduleAssemblyFile(componentType));

        const auto result = _assemblies.emplace(componentType, std::move(description));

        return result.first->second;
    }

private:
    const ComponentPathResolver& _paths;
    const AssemblyDescriptionLoader& _loader;

    std::unordered_map<std::string, AssemblyDescription> _assemblies;
};
