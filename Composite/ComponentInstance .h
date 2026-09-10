#pragma once
#include <string>
#include <memory>
#include "IContainer.h"
#include "IInstance.h"

class ComponentInstance final : public IInstance
{
public:
    ComponentInstance(std::string name, std::unique_ptr<IContainer> container)
        : _name(std::move(name))
        , _container(std::move(container))
    {
        if (_container == nullptr)
        {
            throw std::logic_error("Container nul pour " + _name);
        }
    }

    const std::string& name() const override
    {
        return _name;
    }

    IInputPort* findInputPort(const std::string& operation) override
    {
        return _container->inputPorts().find(operation);
    }

    IOutputPort* findOutputPort(const std::string& operation) override
    {
        return _container->outputPorts().find(operation);
    }

    std::vector<InputPortEntry> inputPorts() override
    {
        std::vector<InputPortEntry> result;

        for (const auto& entry : _container->inputPorts().ports())
        {
            result.push_back({entry.first, entry.second});
        }

        return result;
    }

    std::vector<OutputPortEntry> outputPorts() override
    {
        std::vector<OutputPortEntry> result;

        for (const auto& entry : _container->outputPorts().ports())
        {
            result.push_back({entry.first, entry.second});
        }

        return result;
    }

    IContainer& container()
    {
        return *_container;
    }

private:
    std::string _name;
    std::unique_ptr<IContainer> _container;
};
