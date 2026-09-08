#pragma once

#include <string>
#include <vector>

enum class InstanceKind
{
    Component,
    Composite
};

struct InstanceDescription
{
    std::string name;
    std::string type;
    std::string implementation;
    InstanceKind kind = InstanceKind::Component;
};

struct PortDescription
{
    std::string name;
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

struct InputDelegationDescription
{
    std::string compositeOperation;
    EndpointDescription component;
};

struct OutputDelegationDescription
{
    EndpointDescription component;
    std::string compositeOperation;
};

struct ImplicitLinkDescription
{
    std::string instance = "*";
};

struct ScopeDescription
{
    std::vector<InstanceDescription> instances;
    std::vector<EventLinkDescription> eventLinks;
    std::vector<ImplicitLinkDescription> implicitLinks;
};

struct CompositeDescription : ScopeDescription
{
    std::string type;

    std::vector<PortDescription> inputPorts;
    std::vector<PortDescription> outputPorts;

    std::vector<InputDelegationDescription> inputDelegations;
    std::vector<OutputDelegationDescription> outputDelegations;
};

struct ArchitectureDescription : ScopeDescription
{
    std::vector<CompositeDescription> composites;
};
