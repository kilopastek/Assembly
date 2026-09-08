#pragma once

#include <stdexcept>
#include "ArchitectureDescription.h"
#include "CompositeInstance.h"
#include "ExplicitConnectionRegistry.h"

class ImplicitLinkResolver
{
public:
    void resolve(
        CompositeInstance& scope,
        const ImplicitLinkDescription& rule,
        const ExplicitConnectionRegistry& explicitConnections)
    {
        const auto instances = scope.instances();

        for (IInstance* sender : instances)
        {
            if (!matchesInstance(rule.instance, sender->name()))
            {
                continue;
            }

            for (IOutputPort* output : sender->outputPorts())
            {
                if (explicitConnections.isExplicitSender(
                        sender->name(),
                        output->operationName()))
                {
                    continue;
                }

                for (IInstance* receiver : instances)
                {
                    if (receiver == sender)
                    {
                        continue;
                    }

                    IInputPort* input =
                        receiver->findInputPort(
                            output->operationName());

                    if (input == nullptr)
                    {
                        continue;
                    }

                    if (explicitConnections.isExplicitReceiver(
                            receiver->name(),
                            input->operationName()))
                    {
                        continue;
                    }

                    output->connectTo(*input);
                }
            }
        }
    }

private:
    bool matchesInstance(
        const std::string& filter,
        const std::string& instanceName) const
    {
        return filter == "*" ||
               filter == instanceName;
    }
};