
#include <pugixml.hpp>
#include <stdexcept>
#include "ArchitectureXmlLoader.h"

ArchitectureDescription ArchitectureXmlLoader::load(const std::string& filename)
{
    pugi::xml_document document;

    const auto result = document.load_file(filename.c_str());

    if (!result)
    {
        throw std::runtime_error("Unable to load architecture XML: " + filename);
    }

    ArchitectureDescription architecture;

    const auto root = document.child("architecture");

    const auto composites = root.child("composites");

    for (const auto compositeNode : composites.children("composite"))
    {
        architecture.composites.push_back(loadComposite(compositeNode));
    }

    loadInstances(root, architecture);

    loadLinks(root, architecture);

    loadImplicitLinks(root, architecture);

    return architecture;
}

void ArchitectureXmlLoader::loadInstances( const pugi::xml_node& parent, ScopeDescription& scope)
{
    const auto instances = parent.child("instances");

    for (const auto node : instances.children("instance"))
    {
        InstanceDescription description;

        description.name = node.attribute("name").as_string();
        description.type = node.attribute("componentType").as_string();
        description.implementation = node.attribute("impl").as_string();

        description.kind = InstanceKind::Component;

        scope.instances.push_back(std::move(description));
    }

    for (const auto node : instances.children("compositeInstance"))
    {
        InstanceDescription description;

        description.name = node.attribute("name").as_string();
        description.type = node.attribute("compositeType").as_string();
        description.kind = InstanceKind::Composite;

        scope.instances.push_back(std::move(description));
    }
}

void ArchitectureXmlLoader::loadLinks(const pugi::xml_node& parent, ScopeDescription& scope)
{
    auto links = parent.child("link");

    if (!links)
    {
        links = parent.child("links");
    }

    if (!links)
    {
        return;
    }

    for (const auto eventLink : links.children("eventLink"))
    {
        EventLinkDescription link;

        const auto sender = eventLink.child("sender");

        link.sender.instanceName = sender.attribute("instanceName").as_string();
        link.sender.operation = sender.attribute("operation").as_string();

        const auto receiver = eventLink.child("receiver");

        link.receiver.instanceName = receiver.attribute("instanceName").as_string();
        link.receiver.operation = receiver.attribute("operation").as_string();

        scope.eventLinks.push_back(std::move(link));
    }
}

void ArchitectureXmlLoader::loadImplicitLinks(const pugi::xml_node& parent, ScopeDescription& scope)
{
    const auto implicitLinks = parent.child("implicitLinks");

    if (!implicitLinks)
    {
        return;
    }

    for (const auto operation : implicitLinks.children("operations"))
    {
        ImplicitLinkDescription link;

        link.instance = operation.attribute("instance").as_string("*");

        scope.implicitLinks.push_back(std::move(link));
    }
}

CompositeDescription ArchitectureXmlLoader::loadComposite(const pugi::xml_node& node)
{
    CompositeDescription description;

    description.type = node.attribute("type").as_string();

    const auto ports = node.child("ports");

    for (const auto port : ports.children("input"))
    {
        description.inputPorts.push_back(PortDescription{port.attribute("operation").as_string()});
    }

    for (const auto port : ports.children("output"))
    {
        description.outputPorts.push_back(PortDescription{port.attribute("operation").as_string()});
    }

    loadInstances(node, description);

    loadLinks(node, description);

    loadImplicitLinks(node, description);

    const auto delegations = node.child("delegations");

    for (const auto input : delegations.children("input"))
    {
        InputDelegationDescription delegation;

        delegation.compositeOperation = input.attribute("compositeOperation").as_string();
        delegation.component.instanceName = input.attribute("instanceName").as_string();
        delegation.component.operation = input.attribute("operation").as_string();

        description.inputDelegations.push_back(std::move(delegation));
    }

    for (const auto output : delegations.children("output"))
    {
        OutputDelegationDescription delegation;

        delegation.component.instanceName = output.attribute("instanceName").as_string();
        delegation.component.operation = output.attribute("operation").as_string();
        delegation.compositeOperation = output.attribute("compositeOperation").as_string();

        description.outputDelegations.push_back(std::move(delegation));
    }

    return description;
}
