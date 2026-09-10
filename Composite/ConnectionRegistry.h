#pragma once

#include <set>
#include <tuple>
#include <string>
#include <stdexcept>
#include "CompositeInstance.h"
#include "IInstance.h"
#include "Port.h"
    

struct PortId
{
    std::string instance;
    std::string operation;

    bool operator<(const PortId& other) const
    {
        return std::tie(instance, operation) < std::tie(other.instance, other.operation);
    }
};


class ExplicitConnectionRegistry
{
public:
    void addSender(const std::string& instance, const std::string& operation)
    {
        _senders.insert({ instance, operation });
    }

    void addReceiver(const std::string& instance, const std::string& operation)
    {
        _receivers.insert({ instance, operation });
    }

    bool isExplicitSender(const std::string& instance, const std::string& operation) const
    {
        return _senders.find({ instance, operation }) != _senders.end();
    }

    bool isExplicitReceiver(const std::string& instance, const std::string& operation) const
    {
        return _receivers.find({ instance, operation }) != _receivers.end();
    }

private:
    std::set<PortId> _senders;
    std::set<PortId> _receivers;
};

class LinkResolver
{
public:
    void connect(CompositeInstance& scope, const EventLinkDescription& link, ExplicitConnectionRegistry& explicitConnections)
    {
        IInstance* sender = scope.findInstance(link.sender.instanceName);

        if (sender == nullptr)
        {
            throw std::logic_error("Instance sender inconnue : " + link.sender.instanceName);
        }

        IInstance* receiver = scope.findInstance(link.receiver.instanceName);

        if (receiver == nullptr)
        {
            throw std::logic_error("Instance receiver inconnue : " + link.receiver.instanceName);
        }

        IOutputPort* output = sender->findOutputPort(link.sender.operation);

        if (output == nullptr)
        {
            throw std::logic_error("Output inconnu : " + link.sender.instanceName + "." + link.sender.operation);
        }

        IInputPort* input = receiver->findInputPort(link.receiver.operation);

        if (input == nullptr)
        {
            throw std::logic_error("Input inconnu : " + link.receiver.instanceName + "." + link.receiver.operation);
        }

        if (output->signature() != input->signature())
        {
            throw std::logic_error("Signatures incompatibles : " + link.sender.instanceName + "." + link.sender.operation + " -> " + link.receiver.instanceName + "." + link.receiver.operation);
        }

        output->connectTo(*input);

        explicitConnections.addSender(link.sender.instanceName, link.sender.operation);

        explicitConnections.addReceiver(link.receiver.instanceName, link.receiver.operation);
    }
};

class ImplicitLinkResolver
{
public:
    void resolve(CompositeInstance& scope, const ImplicitLinkDescription& rule, const ExplicitConnectionRegistry& explicitConnections)
    {
        const auto instances = scope.instances();

        for (IInstance* sender : instances)
        {
            const auto outputs = sender->outputPorts();

            for (const auto& output : outputs)
            {
                /*
                 * Si ce port a déjà été explicitement
                 * utilisé comme sender, il ne participe
                 * pas à l'implicite.
                 */
                if (explicitConnections.isExplicitSender(sender->name(), output.name))
                {
                    continue;
                }

                for (IInstance* receiver : instances)
                {
                    if (receiver == sender)
                    {
                        continue;
                    }

                    if (!matchesRule(rule, *sender, *receiver))
                    {
                        continue;
                    }

                    /*
                     * Le coeur du mécanisme :
                     *
                     * même nom => candidat.
                     */
                    IInputPort* input = receiver->findInputPort(output.name);

                    if (input == nullptr)
                    {
                        continue;
                    }

                    if (explicitConnections.isExplicitReceiver(receiver->name(), output.name))
                    {
                        continue;
                    }

                    if (output.port->signature() != input->signature())
                    {
                        throw std::logic_error(
                            "Ports de meme nom mais "
                            "signatures incompatibles : " +
                            sender->name() +
                            "." +
                            output.name +
                            " -> " +
                            receiver->name() +
                            "." +
                            output.name);
                    }

                    output.port->connectTo(*input);
                }
            }
        }
    }

private:
    bool matchesRule(const ImplicitLinkDescription& rule, const IInstance& sender, const IInstance& receiver) const
    {
        if (rule.instance == "*")
        {
            return true;
        }

        return sender.name() == rule.instance || receiver.name() == rule.instance;
    }
};

class CompositeInterfaceResolver
{
public:
    void resolve(CompositeInstance& composite, const ComponentDescription& description)
    {
        for (const auto& operation : description.operations)
        {
            if (operation.direction == OperationDirection::Received)
            {
                exposeInput(composite, operation.name);
            }
            else
            {
                exposeOutput(composite, operation.name);
            }
        }
    }

private:
    void exposeInput(CompositeInstance& composite, const std::string& operation)
    {
        IInputPort* found = nullptr;

        for (IInstance* instance : composite.instances())
        {
            IInputPort* candidate = instance->findInputPort(operation);

            if (candidate == nullptr)
            {
                continue;
            }

            if (found != nullptr)
            {
                throw std::logic_error("Plusieurs inputs internes correspondent a l'input expose " +
                    composite.name() +
                    "." +
                    operation);
            }

            found = candidate;
        }

        if (found == nullptr)
        {
            throw std::logic_error("Impossible de trouver l'input interne pour " +
                composite.name() +
                "." +
                operation);
        }

        composite.exposeInput(operation, *found);
    }

    void exposeOutput(CompositeInstance& composite, const std::string& operation)
    {
        IOutputPort* found = nullptr;

        for (IInstance* instance : composite.instances())
        {
            IOutputPort* candidate = instance->findOutputPort(operation);

            if (candidate == nullptr)
            {
                continue;
            }

            if (found != nullptr)
            {
                throw std::logic_error("Plusieurs outputs internes correspondent a l'output expose " +
                    composite.name() +
                    "." +
                    operation);
            }

            found = candidate;
        }

        if (found == nullptr)
        {
            throw std::logic_error("Impossible de trouver l'output interne pour " +
                composite.name() +
                "." +
                operation);
        }

        composite.exposeOutput(operation, *found);
    }
};
