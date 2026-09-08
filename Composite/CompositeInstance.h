#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "IInstance.h"
#include "Port.h"

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
        const std::string name = instance->name();

        const auto result =
            _instances.emplace(
                name,
                std::move(instance));

        if (!result.second)
        {
            throw std::logic_error(
                "Instance already registered: " + name);
        }
    }

    IInstance* findInstance(
        const std::string& name)
    {
        const auto it =
            _instances.find(name);

        if (it == _instances.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void exposeInputPort(
        const std::string& compositeOperation,
        IInputPort& internalPort)
    {
        const auto result =
            _inputs.emplace(
                compositeOperation,
                &internalPort);

        if (!result.second)
        {
            throw std::logic_error(
                "Composite input already exposed: " +
                _name + "." +
                compositeOperation);
        }
    }

    void exposeOutputPort(
        const std::string& compositeOperation,
        IOutputPort& internalPort)
    {
        const auto result =
            _outputs.emplace(
                compositeOperation,
                &internalPort);

        if (!result.second)
        {
            throw std::logic_error(
                "Composite output already exposed: " +
                _name + "." +
                compositeOperation);
        }
    }

    IInputPort* findInputPort(
        const std::string& operation) override
    {
        const auto it =
            _inputs.find(operation);

        if (it == _inputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    IOutputPort* findOutputPort(
        const std::string& operation) override
    {
        const auto it =
            _outputs.find(operation);

        if (it == _outputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::vector<IInputPort*> inputPorts() override
    {
        std::vector<IInputPort*> result;
        result.reserve(_inputs.size());

        for (const auto& entry : _inputs)
        {
            result.push_back(entry.second);
        }

        return result;
    }

    std::vector<IOutputPort*> outputPorts() override
    {
        std::vector<IOutputPort*> result;
        result.reserve(_outputs.size());

        for (const auto& entry : _outputs)
        {
            result.push_back(entry.second);
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

    std::unordered_map<
        std::string,
        std::unique_ptr<IInstance>> _instances;

    /*
     * Ces ports ne sont PAS possédés par le composite.
     * Ils appartiennent aux containers des composants.
     */
    std::unordered_map<
        std::string,
        IInputPort*> _inputs;

    std::unordered_map<
        std::string,
        IOutputPort*> _outputs;
};
