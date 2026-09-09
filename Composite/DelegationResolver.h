#pragma once

#include <stdexcept>
#include "ArchitectureDescription.h"
#include "CompositeInstance.h"

class IOutputPort;
class IInstance;
class IInputPort;

class DelegationResolver
{
public:
    void exposeInput(CompositeInstance& composite, const InputDelegationDescription& delegation)
    {
        IInstance* instance = composite.findInstance(delegation.component.instanceName);

        if (instance == nullptr)
        {
            throw std::logic_error("Unknown internal instance: " + delegation.component.instanceName);
        }

        IInputPort* input = instance->findInputPort(delegation.component.operation);

        if (input == nullptr)
        {
            throw std::logic_error("Unknown internal input: " + delegation.component.instanceName + "." + delegation.component.operation);
        }

        composite.exposeInputPort(delegation.compositeOperation, *input);
    }

    void exposeOutput(CompositeInstance& composite, const OutputDelegationDescription& delegation)
    {
        IInstance* instance = composite.findInstance(delegation.component.instanceName);

        if (instance == nullptr)
        {
            throw std::logic_error("Unknown internal instance: " + delegation.component.instanceName);
        }

        IOutputPort* output = instance->findOutputPort(delegation.component.operation);

        if (output == nullptr)
        {
            throw std::logic_error("Unknown internal output: " + delegation.component.instanceName + "." + delegation.component.operation);
        }

        composite.exposeOutputPort(delegation.compositeOperation, *output);
    }
};
