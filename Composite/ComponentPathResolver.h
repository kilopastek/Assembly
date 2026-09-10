#pragma once

#include <filesystem>
#include <string>


class ComponentPathResolver
{
public:
    ComponentPathResolver(std::filesystem::path rootDirectory, std::filesystem::path modulesDirectory)
        : _rootDirectory(std::move(rootDirectory))
        , _modulesDirectory(std::move(modulesDirectory))
    {
    }

    std::filesystem::path rootComponentFile(const std::string& componentType) const
    {
        return _rootDirectory / (componentType + ".comp.xml");
    }

    std::filesystem::path rootAssemblyFile(const std::string& componentType) const
    {
        return _rootDirectory / "composite" / (componentType + ".composite.assembly.xml");
    }

    std::filesystem::path moduleComponentFile(const std::string& componentType) const
    {
        return _modulesDirectory / componentType / (componentType + ".comp.xml");
    }

    std::filesystem::path moduleAssemblyFile(const std::string& componentType) const
    {
        return _modulesDirectory / componentType / "composite" / (componentType + ".composite.assembly.xml");
    }

private:
    std::filesystem::path _rootDirectory;
    std::filesystem::path _modulesDirectory;
};
