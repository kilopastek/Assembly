#pragma once

#include <string>
#include <vector>

enum class OperationDirection
{
    Sent,
    Received
};


struct ParameterDescription
{
    std::string name;
    std::string type;
};


struct OperationDescription
{
    std::string name;

    OperationDirection direction;

    std::vector<ParameterDescription> parameters;
};


struct ComponentDescription
{
    std::string name;

    std::vector<OperationDescription> operations;
};


struct InstanceDescription
{
    std::string name;
    std::string componentType;
    std::string implementation;

    bool isComposite() const noexcept
    {
        return implementation == "composite";
    }
};


struct EndpointDescription
{
    std::string instanceName;
    std::string operation;
};


struct EventLinkDescription
{
    EndpointDescription sender;
    EndpointDescription receiver;
};


struct ImplicitLinkDescription
{
    std::string instance = "*";
};


struct AssemblyDescription
{
    std::vector<InstanceDescription> instances;

    std::vector<EventLinkDescription> eventLinks;

    std::vector<ImplicitLinkDescription> implicitLinks;
};
