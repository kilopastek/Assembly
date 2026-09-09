#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class IOutputPort;

class OutputPortRegistry
{
public:
    void add(const std::string& operation, IOutputPort& port)
    {
        const auto result = _ports.emplace(operation, &port);

        if (!result.second)
        {
            throw std::logic_error("Output port already registered: " + operation);
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

    std::vector<std::pair<std::string, IOutputPort*>> ports() const
    {
        std::vector<std::pair<std::string, IOutputPort*>> result;

        result.reserve(_ports.size());

        for (const auto& entry : _ports)
        {
            result.emplace_back(entry.first, entry.second);
        }

        return result;
    }

private:
    std::unordered_map<std::string, IOutputPort*> _ports;
};
