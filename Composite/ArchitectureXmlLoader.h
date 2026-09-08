#pragma once

#include <string>
#include "ArchitectureDescription.h"

namespace pugi
{
    class xml_node;
}

class ArchitectureXmlLoader
{
public:
    ArchitectureDescription load(const std::string& filename);

private:
    void loadInstances(const pugi::xml_node& parent, ScopeDescription& scope);

    void loadLinks(const pugi::xml_node& parent, ScopeDescription& scope);

    void loadImplicitLinks(const pugi::xml_node& parent, ScopeDescription& scope);

    CompositeDescription loadComposite(const pugi::xml_node& node);
};
