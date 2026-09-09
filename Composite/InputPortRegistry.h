#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class IInputPort;

class InputPortRegistry
{
public:
    void add(const std::string& operation, IInputPort& port)
    {
        const auto result = _ports.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Input port already registered: " + operation);
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

    std::vector<std::pair<std::string, IInputPort*>> ports() const
    {
        std::vector<std::pair<std::string, IInputPort*>> result;

        result.reserve(_ports.size());

        for (const auto& entry : _ports)
        {
            result.emplace_back(entry.first, entry.second);
        }

        return result;
    }

private:
    std::unordered_map<std::string, IInputPort*> _ports;
};
