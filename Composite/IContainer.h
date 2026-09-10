#pragma once
#include "PortRegistry.h"

class IContainer
{
public:
    virtual ~IContainer() = default;

    virtual InputPortRegistry& inputPorts() = 0;
    virtual OutputPortRegistry& outputPorts() = 0;
};
