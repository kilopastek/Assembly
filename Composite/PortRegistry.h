#pragma once

#include <string>
#include <unordered_map>
#include "Port.h"

class InputPortRegistry
{
public:
    void add(const std::string& operation, IInputPort& port)
    {
        const auto result = _ports.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Input port deja enregistre : " + operation);
        }
    }

    IInputPort* find(const std::string& operation) const
    {
        const auto it = _ports.find(operation);

        if (it == _ports.end())
        {
            return nullptr;
        }

        return it->second;
    }

    const std::unordered_map<std::string, IInputPort*>& ports() const noexcept
    {
        return _ports;
    }

private:
    std::unordered_map<std::string, IInputPort*> _ports;
};


class OutputPortRegistry
{
public:
    void add(const std::string& operation, IOutputPort& port)
    {
        const auto result = _ports.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Output port deja enregistre : " + operation);
        }
    }

    IOutputPort* find(const std::string& operation) const
    {
        const auto it = _ports.find(operation);

        if (it == _ports.end())
        {
            return nullptr;
        }

        return it->second;
    }

    const std::unordered_map<std::string, IOutputPort*>& ports() const noexcept
    {
        return _ports;
    }

private:
    std::unordered_map<std::string, IOutputPort*> _ports;
};