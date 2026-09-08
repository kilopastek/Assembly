#pragma once

#include <memory>
#include <string>
#include "ComponentInstance.h"

class InstanceDescription;

class IComponentFactory
{
public:
    virtual ~IComponentFactory() = default;

    virtual std::unique_ptr<IInstance> create(const InstanceDescription& description) = 0;
};
