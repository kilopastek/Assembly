#pragma once

#include <stdexcept>
#include "ArchitectureDescription.h"
#include "CompositeInstance.h"
#include "Port.h"

class DelegationResolver
{
public:
    void connectInput(
        CompositeInstance& composite,
        const InputDelegationDescription& delegation)
    {
        IInstance* internalInstance =
            composite.findInstance(
                delegation.component.instanceName);

        if (internalInstance == nullptr)
        {
            throw std::logic_error(
                "Unknown internal instance: " +
                delegation.component.instanceName);
        }

        IInputPort* input =
            internalInstance->findInputPort(
                delegation.component.operation);

        if (input == nullptr)
        {
            throw std::logic_error(
                "Unknown internal input port: " +
                delegation.component.instanceName +
                "." +
                delegation.component.operation);
        }

        composite.exposeInputPort(
            delegation.compositeOperation,
            *input);
    }

    void connectOutput(
    CompositeInstance& composite,
    const OutputDelegationDescription& delegation)
    {
        IInstance* internalInstance =
            composite.findInstance(
                delegation.component.instanceName);

        if (internalInstance == nullptr)
        {
            throw std::logic_error(
                "Unknown internal instance: " +
                delegation.component.instanceName);
        }

        IOutputPort* output =
            internalInstance->findOutputPort(
                delegation.component.operation);

        if (output == nullptr)
        {
            throw std::logic_error(
                "Unknown internal output port: " +
                delegation.component.instanceName +
                "." +
                delegation.component.operation);
        }

        composite.exposeOutputPort(
            delegation.compositeOperation,
            *output);
    }
};
