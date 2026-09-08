#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "IInstance.h"
#include "Port.h"

class ComponentInstance final : public IInstance
{
public:
    explicit ComponentInstance(std::string name)
        : _name(std::move(name))
    {
    }

    const std::string& name() const override
    {
        return _name;
    }

    void registerInputPort(const std::string& operation, IInputPort& port)
    {
        const auto result = _inputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Input port already registered: " + _name + "." + operation);
        }
    }

    void registerOutputPort(const std::string& operation, IOutputPort& port)
    {
        const auto result = _outputs.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Output port already registered: " + _name + "." + operation);
        }
    }

    IInputPort* findInputPort(
        const std::string& operation) override
    {
        const auto it = _inputs.find(operation);

        if (it == _inputs.end())
        {
            return nullptr;
        }

        return it->second;
    }

    IOutputPort* findOutputPort(
        const std::string& operation) override
    {
        const auto it = _outputs.find(operation);

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

private:
    std::string _name;

    std::unordered_map<std::string, IInputPort*> _inputs;
    std::unordered_map<std::string, IOutputPort*> _outputs;
};
