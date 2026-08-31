#include <pugixml.hpp>
#include "ArchitectureDescription.h"
#include <stdexcept>

class ArchitectureXmlLoader
{
public:
    ArchitectureDescription load(const std::string& filename) const
    {
        pugi::xml_document document;

        const pugi::xml_parse_result result = document.load_file(filename.c_str());

        if (!result)
        {
            throw std::runtime_error("Impossible de charger le fichier XML '" + filename + "' : " + result.description());
        }

        ArchitectureDescription architecture;

        const pugi::xml_node root = document.child("architecture");

        if (!root)
        {
            throw std::runtime_error("Noeud <architecture> absent");
        }

        loadInstances(root.child("instances"), architecture);

        loadLinks(root.child("links"), architecture);

        return architecture;
    }

private:
    static std::string requireAttribute(const pugi::xml_node& node, const char* attributeName)
    {
        const pugi::xml_attribute attribute = node.attribute(attributeName);

        if (!attribute)
        {
            throw std::runtime_error(std::string("Attribut '") + attributeName + "' absent dans <" + node.name() + ">" );
        }

        return attribute.as_string();
    }

    static void loadInstances(const pugi::xml_node& instancesNode, ArchitectureDescription& architecture)
    {
        for (const pugi::xml_node instance : instancesNode.children("instance"))
        {
            ComponentInstanceDescription description;

            description.instanceName = requireAttribute(instance, "name");

            description.componentType = requireAttribute(instance, "componentType");

            architecture.addInstance(std::move(description));
        }
    }

    static void loadLinks(const pugi::xml_node& linksNode, ArchitectureDescription& architecture)
    {
        for (const pugi::xml_node eventLink : linksNode.children("eventLink"))
        {
            const pugi::xml_node sender = eventLink.child("sender");

            const pugi::xml_node receiver = eventLink.child("receiver");

            if (!sender || !receiver)
            {
                throw std::runtime_error("eventLink incomplet");
            }

            ComponentLinkDescription link;

            link.senderInstance = requireAttribute(sender, "instanceName");

            link.senderPort = requireAttribute(sender, "operation");

            link.receiverInstance = requireAttribute(receiver, "instanceName");

            link.receiverPort = requireAttribute(receiver, "operation");

            architecture.addLink(std::move(link));
        }
    }
};