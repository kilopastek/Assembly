#pragma once

#include <pugixml.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "AssemblyDescription.h"

class AssemblyDescriptionLoader
{
public:
    AssemblyDescription load(const std::filesystem::path& file) const
    {
        pugi::xml_document document;

        const auto result = document.load_file(file.string().c_str());

        if (!result)
        {
            throw std::runtime_error("Impossible de charger : " + file.string() + " : " + result.description());
        }

        AssemblyDescription description;

        const auto root = document.document_element();

        parseInstances(root, description);

        parseEventLinks(root, description);

        parseImplicitLinks(root, description);

        return description;
    }

private:
    void parseInstances(const pugi::xml_node& root, AssemblyDescription& description) const
    {
        const pugi::xml_node instances = root.child("instances");

        if (!instances)
        {
            return;
        }

        for (const auto& node : instances.children("instance"))
        {
            InstanceDescription instance;

            instance.name = node.attribute("name").as_string();

            instance.componentType = node.attribute("componentType").as_string();

            instance.implementation = node.attribute("implementation").as_string();

            if (instance.name.empty())
            {
                throw std::logic_error("Instance sans name");
            }

            if (instance.componentType.empty())
            {
                throw std::logic_error("Instance " + instance.name + " sans componentType");
            }

            if (instance.implementation.empty())
            {
                throw std::logic_error("Instance " + instance.name + " sans implementation");
            }

            description.instances.push_back(std::move(instance));
        }
    }

    void parseEventLinks(const pugi::xml_node& root, AssemblyDescription& description) const
    {
        /*
         * Suivant le XML réel, eventLink peut être
         * directement sous root ou sous <links>.
         *
         * Ici on accepte les deux.
         */

        parseEventLinksFromNode(root, description);

        const auto links = root.child("links");

        if (links)
        {
            parseEventLinksFromNode(links, description);
        }

        const auto link = root.child("link");

        if (link)
        {
            parseEventLinksFromNode(link, description);
        }
    }

    void parseEventLinksFromNode(const pugi::xml_node& parent, AssemblyDescription& description) const
    {
        for (const auto& node : parent.children("eventLink"))
        {
            EventLinkDescription link;

            const auto sender = node.child("sender");

            const auto receiver = node.child("receiver");

            if (!sender || !receiver)
            {
                throw std::logic_error("eventLink invalide");
            }

            link.sender.instanceName = sender.attribute("instanceName").as_string();

            link.sender.operation = sender.attribute("operation").as_string();

            link.receiver.instanceName = receiver.attribute("instanceName").as_string();

            link.receiver.operation = receiver.attribute("operation").as_string();

            description.eventLinks.push_back(std::move(link));
        }
    }

    void parseImplicitLinks(const pugi::xml_node& root, AssemblyDescription& description) const
    {
        const auto implicitLinks = root.child("implicitLinks");

        if (!implicitLinks)
        {
            return;
        }

        for (const auto& node : implicitLinks.children("operations"))
        {
            ImplicitLinkDescription rule;

            rule.instance = node.attribute("instance").as_string("*");

            description.implicitLinks.push_back(std::move(rule));
        }
    }
};
