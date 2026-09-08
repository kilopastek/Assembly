#pragma once

#include <stdexcept>
#include <string>
#include "ArchitectureDescription.h"
#include "CompositeInstance.h"
#include "ExplicitConnectionRegistry.h"

class LinkResolver
{
public:
    void connectExplicit(CompositeInstance& scope, const EventLinkDescription& link, ExplicitConnectionRegistry& explicitConnections)
    {
        IInstance* senderInstance = scope.findInstance(link.sender.instanceName);

        if (senderInstance == nullptr)
        {
            throw std::logic_error("Unknown sender instance: " + link.sender.instanceName);
        }

        IInstance* receiverInstance = scope.findInstance(link.receiver.instanceName);

        if (receiverInstance == nullptr)
        {
            throw std::logic_error("Unknown receiver instance: " + link.receiver.instanceName);
        }

        IOutputPort* output = senderInstance->findOutputPort(link.sender.operation);

        if (output == nullptr)
        {
            throw std::logic_error("Unknown output port: " + link.sender.instanceName + "." + link.sender.operation);
        }

        IInputPort* input = receiverInstance->findInputPort(link.receiver.operation);

        if (input == nullptr)
        {
            throw std::logic_error("Unknown input port: " + link.receiver.instanceName + "." + link.receiver.operation);
        }

        connect(*output, *input, link);

        explicitConnections.addSender(link.sender.instanceName, link.sender.operation);
        explicitConnections.addReceiver(link.receiver.instanceName, link.receiver.operation);
    }

private:
    void connect(IOutputPort& output, IInputPort& input, const EventLinkDescription& link)
    {
        /*
         * Cette comparaison permet de produire une erreur
         * plus explicite avant le dynamic_cast effectué
         * dans OutputPort<Signature>::connectTo().
         */
        if (output.signature() != input.signature())
        {
            throw std::logic_error(
                "Incompatible port signatures: " +
                link.sender.instanceName +
                "." +
                link.sender.operation +
                " -> " +
                link.receiver.instanceName +
                "." +
                link.receiver.operation);
        }

        output.connectTo(input);
    }
};