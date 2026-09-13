#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>

struct ComponentInstanceDescription
{
    std::string instanceName;      // Nom complètement qualifié
    std::string componentType;
};

struct ComponentConnectionDescription
{
    std::string senderInstance;
    std::string senderPort;

    std::string receiverInstance;
    std::string receiverPort;
};

class ArchitectureDescription
{
public:
    void addInstance(ComponentInstanceDescription instance)
    {
        const auto name = instance.instanceName;

        const auto [it, inserted] = _instances.emplace(name, std::move(instance));

        if (!inserted)
        {
            throw std::runtime_error("Instance déjà déclarée : " + name);
        }
    }

    void addConnection(ComponentConnectionDescription connection)
    {
        _connections.emplace_back(std::move(connection));
    }

    const auto& getInstances() const noexcept
    {
        return _instances;
    }

    const auto& getConnections() const noexcept
    {
        return _connections;
    }

private:
    std::unordered_map<std::string, ComponentInstanceDescription> _instances;

    std::vector<ComponentConnectionDescription> _connections;
};
