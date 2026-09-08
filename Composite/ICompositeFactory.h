#pragma once

#include <memory>
#include <string>

class CompositeInstance;
class CompositeDescription;

class ICompositeFactory
{
public:
    virtual ~ICompositeFactory() = default;

    virtual std::unique_ptr<CompositeInstance> create(const std::string& instanceName, const CompositeDescription& description) = 0;
};
