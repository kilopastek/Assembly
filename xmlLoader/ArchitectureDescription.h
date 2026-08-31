#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

struct ComponentInstanceDescription
{
    std::string instanceName;
    std::string componentType;
};

struct ComponentLinkDescription
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
        const std::string name = instance.instanceName;

        const auto [iterator, inserted] = _instances.emplace(name, std::move(instance));

        if (!inserted)
        {
            throw std::logic_error("Instance dupliquée : " + name);
        }
    }

    const ComponentInstanceDescription& getInstance(const std::string& instanceName) const
    {
        const auto iterator = _instances.find(instanceName);

        if (iterator == _instances.end())
        {
            throw std::logic_error("Instance inconnue : " + instanceName);
        }

        return iterator->second;
    }

    const auto& getInstances() const noexcept
    {
        return _instances;
    }

    const auto& getLinks() const noexcept
    {
        return _links;
    }

private:
    std::unordered_map<std::string, ComponentInstanceDescription> _instances;

    std::vector<ComponentLinkDescription> _links;
};