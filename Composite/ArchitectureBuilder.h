#pragma once

#include <memory>
#include <stdexcept>
#include "ArchitectureDescription.h"
#include "CompositeInstance.h"
#include "IComponentFactory.h"
#include "ICompositeFactory.h"
#include "LinkResolver.h"
#include "ImplicitLinkResolver.h"
#include "DelegationResolver.h"
#include "ComponentFactoryRegistry.h"

class ArchitectureBuilder
{
public:
    explicit ArchitectureBuilder(ComponentFactoryRegistry& componentFactories)
        : _componentFactories(componentFactories)
    {
    }

    std::unique_ptr<CompositeInstance> build(const ArchitectureDescription& architecture)
    {
        _architecture = &architecture;

        /*
         * Scope racine synthétique.
         */
        auto root = std::make_unique<CompositeInstance>("$root");

        buildScope(*root, architecture);

        return root;
    }

private:
    void buildScope(CompositeInstance& scope, const ScopeDescription& description)
    {
        /*
         * 1. Création de toutes les instances.
         */
        createInstances(scope, description.instances);

        /*
         * Ce registry est spécifique au scope.
         */
        ExplicitConnectionRegistry explicitConnections;

        /*
         * 2. Liens explicitement décrits dans le XML.
         */
        for (const auto& link : description.eventLinks)
        {
            _linkResolver.connectExplicit(scope, link, explicitConnections);
        }

        /*
         * 3. Résolution des connexions implicites.
         */
        for (const auto& rule : description.implicitLinks)
        {
            _implicitLinkResolver.resolve(scope, rule, explicitConnections);
        }
    }

    void createInstances(CompositeInstance& scope, const std::vector<InstanceDescription>& descriptions)
    {
        for (const auto& description : descriptions)
        {
            if (description.kind == InstanceKind::Component)
            {
                auto instance = _componentFactories.create(description);

                scope.addInstance(std::move(instance));

                continue;
            }

            const CompositeDescription* compositeDescription = findComposite(description.type);

            if (compositeDescription == nullptr)
            {
                throw std::logic_error(
                    "Unknown composite type: " +
                    description.type);
            }

            auto composite = buildComposite(description.name, *compositeDescription);

            scope.addInstance(std::move(composite));
        }
    }

    std::unique_ptr<CompositeInstance> buildComposite(const std::string& instanceName, const CompositeDescription& description)
    {
        /*
         * Pas de factory.
         * Pas de container.
         *
         * Le composite est purement architectural.
         */
        auto composite = std::make_unique<CompositeInstance>(instanceName);

        /*
         * Construction de son propre scope.
         */
        buildScope(*composite, description);

        /*
         * Tous les composants internes existent
         * désormais, donc les aliases peuvent
         * être résolus.
         */
        for (const auto& delegation : description.inputDelegations)
        {
            _delegationResolver.exposeInput(*composite, delegation);
        }

        for (const auto& delegation : description.outputDelegations)
        {
            _delegationResolver.exposeOutput(*composite, delegation);
        }

        return composite;
    }

    const CompositeDescription* findComposite(const std::string& type) const
    {
        if (_architecture == nullptr)
        {
            return nullptr;
        }

        for (const auto& composite : _architecture->composites)
        {
            if (composite.type == type)
            {
                return &composite;
            }
        }

        return nullptr;
    }

private:
    ComponentFactoryRegistry& _componentFactories;

    const ArchitectureDescription* _architecture = nullptr;

    LinkResolver _linkResolver;
    ImplicitLinkResolver _implicitLinkResolver;
    DelegationResolver _delegationResolver;
};
