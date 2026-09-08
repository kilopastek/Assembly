#pragma once

#include <set>
#include <string>
#include <tuple>

struct PortId
{
    std::string instanceName;
    std::string operationName;

    bool operator<(const PortId& other) const
    {
        return std::tie(instanceName, operationName) < std::tie(other.instanceName, other.operationName);
    }
};

class ExplicitConnectionRegistry
{
public:
    void addSender(const std::string& instanceName, const std::string& operationName)
    {
        _senders.insert({ instanceName, operationName });
    }

    void addReceiver(const std::string& instanceName, const std::string& operationName)
    {
        _receivers.insert({ instanceName, operationName });
    }

    bool isExplicitSender(const std::string& instanceName, const std::string& operationName) const
    {
        return _senders.find({ instanceName, operationName }) != _senders.end();
    }

    bool isExplicitReceiver(const std::string& instanceName, const std::string& operationName) const
    {
        return _receivers.find({ instanceName, operationName }) != _receivers.end();
    }

private:
    std::set<PortId> _senders;
    std::set<PortId> _receivers;
};
