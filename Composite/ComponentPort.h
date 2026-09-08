
#pragma once

#include "Port.h"

class ComponentInputPort final : public IInputPort
{
public:
    using Callback = std::function<void(const Event&)>;

    ComponentInputPort(std::string instanceName, std::string operationName, Callback callback)
        : _instanceName(std::move(instanceName))
        , _operationName(std::move(operationName))
        , _callback(std::move(callback))
    {
    }

    const std::string& instanceName() const override
    {
        return _instanceName;
    }

    const std::string& operationName() const override
    {
        return _operationName;
    }

    void receive(const Event& event) override
    {
        if (_callback)
        {
            _callback(event);
        }
    }

private:
    std::string _instanceName;
    std::string _operationName;
    Callback _callback;
};


class ComponentOutputPort final : public IOutputPort
{
public:
    ComponentOutputPort(std::string instanceName, std::string operationName)
        : _instanceName(std::move(instanceName))
        , _operationName(std::move(operationName))
    {
    }

    const std::string& instanceName() const override
    {
        return _instanceName;
    }

    const std::string& operationName() const override
    {
        return _operationName;
    }

    void connect(IInputPort& input) override
    {
        auto it = std::find(_targets.begin(), _targets.end(), &input);

        if (it == _targets.end())
        {
            _targets.push_back(&input);
        }
    }

    void emit(const Event& event)
    {
        for (IInputPort* target : _targets)
        {
            target->receive(event);
        }
    }

private:
    std::string _instanceName;
    std::string _operationName;

    std::vector<IInputPort*> _targets;
};
