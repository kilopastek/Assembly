#pragma once
#include <string>
#include <vector>

enum class ImplementationKind
{
    Component,
    Composite
};

struct ChildInstanceDescription
{
    std::string instanceName;
    std::string componentType;
    ImplementationKind implementationKind;
};

struct LocalConnectionDescription
{
    std::string senderInstance;
    std::string senderPort;

    std::string receiverInstance;
    std::string receiverPort;
};

struct CompositeDefinition
{
    std::string typeName;

    std::vector<ChildInstanceDescription> children;
    std::vector<LocalConnectionDescription> connections;
};
