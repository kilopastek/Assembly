#pragma once

#include "InputPortRegistry.h"
#include "OutputPortRegistry.h"

class IContainer
{
public:
    virtual ~IContainer() = default;

    virtual InputPortRegistry& inputPorts() = 0;
    virtual OutputPortRegistry& outputPorts() = 0;
};
