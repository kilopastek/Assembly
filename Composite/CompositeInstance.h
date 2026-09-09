#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "IInstance.h"

class CompositeInstance final : public IInstance
{
public:
    explicit CompositeInstance(std::string name)
        : _name(std::move(name))
    {
    }

    const std::string& name() const override
    {
        return _name;
    }

    void addInstance(
        std::unique_ptr<IInstance> instance)
    {
        if (instance == nullptr)
        {
            throw std::logic_error("Cannot add null instance");
        }

        const std::string instanceName = instance->name();

        const auto result = _instances.emplace(instanceName, std::move(instance));

        if (!result.second)
        {
            throw std::logic_error("Instance already exists: " + instanceName);
        }
    }

    IInstance* findInstance(const std::string& instanceName)
    {
        const auto it = _instances.find(instanceName);

        if (it == _instances.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void exposeInputPort(const std::string& operation, IInputPort& port)
    {
        const auto result = _inputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Composite input already exposed: " + _name + "." + operation);
        }
    }

    void exposeOutputPort(const std::string& operation, IOutputPort& port)
    {
        const auto result = _outputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Composite output already exposed: " + _name + "." + operation);
        }
    }

    IInputPort* findInputPort(const std::string& operation) override
    {
        const auto it = _inputs.find(operation);

        if (it == _inputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    IOutputPort* findOutputPort(const std::string& operation) override
    {
        const auto it = _outputs.find(operation);

        if (it == _outputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::vector<InputPortEntry> inputPorts() override
    {
        std::vector<InputPortEntry> result;

        result.reserve(_inputs.size());

        for (const auto& entry : _inputs)
        {
            result.push_back({ entry.first, entry.second });
        }

        return result;
    }

    std::vector<OutputPortEntry> outputPorts() override
    {
        std::vector<OutputPortEntry> result;

        result.reserve(_outputs.size());

        for (const auto& entry : _outputs)
        {
            result.push_back({ entry.first, entry.second });
        }

        return result;
    }

    std::vector<IInstance*> instances()
    {
        std::vector<IInstance*> result;

        result.reserve(_instances.size());

        for (auto& entry : _instances)
        {
            result.push_back(entry.second.get());
        }

        return result;
    }

private:
    std::string _name;

    /*
     * Les vraies instances du scope.
     */
    std::unordered_map<std::string, std::unique_ptr<IInstance>> _instances;

    /*
     * Simple table d'alias.
     *
     * Aucun port n'est possédé par le composite.
     */
    std::unordered_map<std::string, IInputPort*> _inputs;

    std::unordered_map<std::string, IOutputPort*> _outputs;
};
