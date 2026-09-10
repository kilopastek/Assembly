#pragma once

#include <memory>

class ArchitectureBuilder
{
public:
    ArchitectureBuilder(
        const ComponentPathResolver& paths,
        const ComponentDescriptionLoader& componentLoader,
        const AssemblyDescriptionLoader& assemblyLoader,
        ComponentDescriptionRegistry& componentDescriptions,
        AssemblyDescriptionRegistry& assemblyDescriptions,
        ComponentBuilderRegistry& componentBuilders)
        : _paths(paths)
        , _componentLoader(componentLoader)
        , _assemblyLoader(assemblyLoader)
        , _componentDescriptions(componentDescriptions)
        , _assemblyDescriptions(assemblyDescriptions)
        , _componentBuilders(componentBuilders)
    {
    }

    std::unique_ptr<CompositeInstance> build(const std::string& rootComponentType)
    {
        /*
         * Cas particulier :
         * le composant initial est situé dans
         * le dossier Composant et est toujours
         * un composite.
         */

        const ComponentDescription interfaceDescription = _componentLoader.load(_paths.rootComponentFile(rootComponentType));

        const AssemblyDescription assemblyDescription = _assemblyLoader.load(_paths.rootAssemblyFile(rootComponentType));

        return buildComposite(rootComponentType, interfaceDescription, assemblyDescription);
    }

private:
    std::unique_ptr<IInstance> buildInstance(const InstanceDescription& description)
    {
        if (!description.isComposite())
        {
            /*
             * C ou CPP :
             * création par le code fourni
             * par le développeur.
             */
            return _componentBuilders.create(description);
        }

        /*
         * Composite :
         *
         * Modules/<type>/<type>.comp.xml
         *
         * +
         *
         * Modules/<type>/composite/
         *     <type>.composite.assembly.xml
         */

        const ComponentDescription& interfaceDescription = _componentDescriptions.getModule(description.componentType);

        const AssemblyDescription& assemblyDescription = _assemblyDescriptions.getModule(description.componentType);

        return buildComposite(description.name, interfaceDescription, assemblyDescription);
    }

    std::unique_ptr<CompositeInstance> buildComposite(
        const std::string& instanceName,
        const ComponentDescription& interfaceDescription,
        const AssemblyDescription& assemblyDescription)
    {
        auto composite = std::make_unique<CompositeInstance>(instanceName);

        /*
         * 1. Toutes les instances internes sont
         *    construites.
         *
         * Les composites enfants sont donc entièrement
         * construits avant de poursuivre.
         */
        for (const auto& instance : assemblyDescription.instances)
        {
            composite->addInstance(buildInstance(instance));
        }

        /*
         * Registry local au scope courant.
         *
         * Un eventLink situé dans ce composite ne
         * concerne que ce composite.
         */
        ExplicitConnectionRegistry explicitConnections;

        /*
         * 2. Liens explicites.
         */
        for (const auto& link : assemblyDescription.eventLinks)
        {
            _linkResolver.connect(*composite, link, explicitConnections);
        }

        /*
         * 3. Liens implicites.
         *
         * Le matching se fait uniquement entre les
         * instances directement visibles dans ce scope.
         */
        for (const auto& rule : assemblyDescription.implicitLinks)
        {
            _implicitLinkResolver.resolve(*composite, rule, explicitConnections);
        }

        /*
         * 4. Interface externe.
         *
         * Une fois toutes les instances internes
         * construites, le composite recherche ses
         * ports exposés.
         */
        _compositeInterfaceResolver.resolve(*composite, interfaceDescription);

        return composite;
    }

private:
    const ComponentPathResolver& _paths;

    const ComponentDescriptionLoader& _componentLoader;

    const AssemblyDescriptionLoader& _assemblyLoader;

    ComponentDescriptionRegistry& _componentDescriptions;

    AssemblyDescriptionRegistry& _assemblyDescriptions;

    ComponentBuilderRegistry& _componentBuilders;

    LinkResolver _linkResolver;

    ImplicitLinkResolver _implicitLinkResolver;

    CompositeInterfaceResolver _compositeInterfaceResolver;
};
