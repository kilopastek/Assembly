#pragma once

#include <string>
#include <filesystem>
#include "ArchitectureDescription.h"
#include "CompositeDefinition.h"

struct CompositePortBinding
{
    std::string compositePort;

    std::string childInstance;
    std::string childPort;
};

struct CompositeDefinition
{
    std::string typeName;

    std::vector<ChildInstanceDescription> children;
    std::vector<LocalConnectionDescription> connections;

    std::vector<CompositePortBinding> inputBindings;
    std::vector<CompositePortBinding> outputBindings;
};

struct PortEndpoint
{
    std::string instanceName;
    std::string portName;
};

class ArchitectureXmlLoader
{
public:
    ArchitectureXmlLoader(std::filesystem::path componentDirectory, std::filesystem::path modulesDirectory)
        : _componentDirectory(std::move(componentDirectory)),
          _modulesDirectory(std::move(modulesDirectory))
    {
    }

    ArchitectureDescription load()
    {
        ArchitectureDescription architecture;

        const std::filesystem::path rootFile = findRootCompositeFile();

        expandComposite(rootFile, "", architecture);

        return architecture;
    }

    void expandComposite(const std::filesystem::path& compositeFile, const std::string& instancePrefix, ArchitectureDescription& architecture)
    {
        const CompositeDefinition composite = loadCompositeDefinition(compositeFile);

        //
        // 1. Développer les instances
        //
        for (const auto& child : composite.children)
        {
            const std::string fullInstanceName = makeInstanceName(instancePrefix, child.instanceName);

            if (child.implementationKind == ImplementationKind::Component)
            {
                architecture.addInstance({ fullInstanceName, child.componentType });
            }
            else
            {
                const auto childCompositeFile = findCompositeFile( child.componentType);

                expandComposite(childCompositeFile, fullInstanceName, architecture);
            }
        }

        //
        // 2. Ajouter les connexions locales
        //
        for (const auto& connection : composite.connections)
        {
            architecture.addConnection({
                    makeInstanceName(instancePrefix, connection.senderInstance),
                    connection.senderPort,
                    makeInstanceName(instancePrefix, connection.receiverInstance),
                    connection.receiverPort
                });
        }
    }

    static std::string makeInstanceName(const std::string& parent, const std::string& child)
    {
        if (parent.empty())
        {
            return child;
        }

        return parent + "." + child;
    }

    PortEndpoint resolveInputEndpoint(const CompositeDefinition& composite, const std::string& compositeInstance, const std::string& portName)
    {
        for (const auto& binding : composite.inputBindings)
        {
            if (binding.compositePort != portName)
            {
                continue;
            }

            return { makeInstanceName(compositeInstance, binding.childInstance), binding.childPort };
        }

        throw std::runtime_error("Port d'entrée composite introuvable : " + compositeInstance + "." + portName);
    }

private:
    std::filesystem::path _componentDirectory;
    std::filesystem::path _modulesDirectory;
};
