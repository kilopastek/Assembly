#pragma once

#include <pugixml.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>


class ComponentDescriptionLoader
{
public:
    ComponentDescription load(const std::filesystem::path& file) const
    {
        pugi::xml_document document;

        const pugi::xml_parse_result result = document.load_file(file.string().c_str());

        if (!result)
        {
            throw std::runtime_error("Impossible de charger : " + file.string() + " : " + result.description());
        }

        const pugi::xml_node root = document.document_element();

        ComponentDescription description;

        description.name = componentNameFromFile(file);

        const pugi::xml_node operations = findOperationsNode(root);

        if (!operations)
        {
            throw std::logic_error("Element <operations> absent dans " + file.string());
        }

        for (const auto& child : operations.children())
        {
            const std::string nodeName = child.name();

            if (nodeName == "eventSent")
            {
                description.operations.push_back(parseOperation(child, OperationDirection::Sent));
            }
            else if (nodeName == "eventReceive")
            {
                description.operations.push_back(parseOperation(child, OperationDirection::Received));
            }
        }

        return description;
    }

private:
    pugi::xml_node findOperationsNode(const pugi::xml_node& root) const
    {
        if (std::string(root.name()) == "operations")
        {
            return root;
        }

        /*
         * Cas :
         *
         * <component>
         *   <operations>
         */
        return root.child("operations");
    }

    OperationDescription parseOperation(const pugi::xml_node& node, OperationDirection direction) const
    {
        OperationDescription result;

        result.direction = direction;

        /*
         * À adapter seulement si le vrai XML
         * n'utilise pas un attribut "name".
         */
        result.name = node.attribute("name").as_string();

        if (result.name.empty())
        {
            const auto nameNode = node.child("name");

            if (nameNode)
            {
                result.name = nameNode.text().as_string();
            }
        }

        if (result.name.empty())
        {
            throw std::logic_error("Operation sans nom");
        }

        for (const auto& child : node.children())
        {
            const std::string childName = child.name();

            /*
             * param0, param1, param2...
             */
            if (childName.rfind("param", 0) != 0)
            {
                continue;
            }

            result.parameters.push_back(parseParameter(child));
        }

        return result;
    }

    ParameterDescription parseParameter(const pugi::xml_node& node) const
    {
        ParameterDescription result;

        result.name = node.name();

        /*
         * Partie dépendante du schéma réel.
         */
        result.type = node.attribute("type").as_string();

        if (result.type.empty())
        {
            const auto type = node.child("type");

            if (type)
            {
                result.type = type.text().as_string();
            }
        }

        return result;
    }

    std::string componentNameFromFile(const std::filesystem::path& file) const
    {
        std::string filename = file.filename().string();

        const std::string suffix = ".comp.xml";

        if (filename.size() > suffix.size() && filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            filename.resize(filename.size() - suffix.size());
        }

        return filename;
    }
};
