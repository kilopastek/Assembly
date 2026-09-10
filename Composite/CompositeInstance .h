#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>
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

    void addInstance(std::unique_ptr<IInstance> instance)
    {
        if (instance == nullptr)
        {
            throw std::logic_error("Tentative d'ajout d'une instance nulle");
        }

        const std::string instanceName = instance->name();

        const auto result = _instances.emplace(instanceName, std::move(instance));

        if (!result.second)
        {
            throw std::logic_error("Instance dupliquee : " + instanceName);
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

    void exposeInput(const std::string& operation, IInputPort& port)
    {
        const auto result = _exposedInputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Input composite duplique : " + _name + "." + operation);
        }
    }

    void exposeOutput(const std::string& operation, IOutputPort& port)
    {
        const auto result = _exposedOutputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Output composite duplique : " + _name + "." + operation);
        }
    }

    IInputPort* findInputPort(const std::string& operation) override
    {
        const auto it = _exposedInputs.find(operation);

        if (it == _exposedInputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    IOutputPort* findOutputPort(const std::string& operation) override
    {
        const auto it = _exposedOutputs.find(operation);

        if (it == _exposedOutputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::vector<InputPortEntry> inputPorts() override
    {
        std::vector<InputPortEntry> result;

        for (const auto& entry : _exposedInputs)
        {
            result.push_back({ entry.first, entry.second });
        }

        return result;
    }

    std::vector<OutputPortEntry> outputPorts() override
    {
        std::vector<OutputPortEntry> result;

        for (const auto& entry : _exposedOutputs)
        {
            result.push_back({ entry.first, entry.second });
        }

        return result;
    }

private:
    std::string _name;

    std::unordered_map<std::string, std::unique_ptr<IInstance>> _instances;

    /*
     * Ce ne sont PAS des registries runtime.
     *
     * Ce sont uniquement des alias vers les ports
     * internes visibles depuis le scope parent.
     */
    std::unordered_map<std::string, IInputPort*> _exposedInputs;

    std::unordered_map<std::string, IOutputPort*> _exposedOutputs;
};
