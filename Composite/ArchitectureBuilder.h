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

class ArchitectureBuilder
{
public:
    ArchitectureBuilder(IComponentFactory& componentFactory, ICompositeFactory& compositeFactory)
        : _componentFactory(componentFactory)
        , _compositeFactory(compositeFactory)
    {
    }

    std::unique_ptr<CompositeInstance> build(const ArchitectureDescription& architecture)
    {
        _architecture = &architecture;

        auto root = std::make_unique<CompositeInstance>("$root");

        buildScope(*root, architecture);

        return root;
    }

private:
    void buildScope(CompositeInstance& scope, const ScopeDescription& description)
    {
        createInstances(scope, description.instances);

        ExplicitConnectionRegistry explicitConnections;

        createExplicitLinks(scope, description.eventLinks, explicitConnections);
        createImplicitLinks(scope, description.implicitLinks, explicitConnections);
    }

    void createInstances(
        CompositeInstance& scope,
        const std::vector<InstanceDescription>& descriptions)
    {
        for (const auto& description : descriptions)
        {
            if (description.kind ==
                InstanceKind::Component)
            {
                auto component =
                    _componentFactory.create(
                        description);

                scope.addInstance(
                    std::move(component));
            }
            else
            {
                const CompositeDescription*
                    compositeDescription =
                        findCompositeDescription(
                            description.type);

                if (compositeDescription == nullptr)
                {
                    throw std::logic_error(
                        "Unknown composite type: " +
                        description.type);
                }

                auto composite =
                    buildComposite(
                        description.name,
                        *compositeDescription);

                scope.addInstance(
                    std::move(composite));
            }
        }
    }

    std::unique_ptr<CompositeInstance>
    ArchitectureBuilder::buildComposite(
        const std::string& instanceName,
        const CompositeDescription& description)
    {
        auto composite =
            std::make_unique<CompositeInstance>(
                instanceName);

        /*
        * Construction des véritables composants internes.
        */
        buildScope(
            *composite,
            description);

        /*
        * Les ports internes existent maintenant.
        * On peut donc publier ceux exposés par le composite.
        */
        for (const auto& delegation :
            description.inputDelegations)
        {
            _delegationResolver.connectInput(
                *composite,
                delegation);
        }

        for (const auto& delegation :
            description.outputDelegations)
        {
            _delegationResolver.connectOutput(
                *composite,
                delegation);
        }

        return composite;
    }

    void createExplicitLinks(CompositeInstance& scope, const std::vector<EventLinkDescription>& links, ExplicitConnectionRegistry& explicitConnections)
    {
        for (const auto& link : links)
        {
            _linkResolver.connectExplicit(
                scope,
                link,
                explicitConnections);
        }
    }

    void createImplicitLinks(CompositeInstance& scope, const std::vector<ImplicitLinkDescription>& links, const ExplicitConnectionRegistry& explicitConnections)
    {
        for (const auto& link : links)
        {
            _implicitLinkResolver.resolve(
                scope,
                link,
                explicitConnections);
        }
    }

    const CompositeDescription* findCompositeDescription(const std::string& type) const
    {
        for (const auto& description : _architecture->composites)
        {
            if (description.type == type)
            {
                return &description;
            }
        }

        return nullptr;
    }

private:
    IComponentFactory& _componentFactory;
    ICompositeFactory& _compositeFactory;

    const ArchitectureDescription* _architecture = nullptr;

    LinkResolver _linkResolver;
    ImplicitLinkResolver _implicitLinkResolver;
    DelegationResolver _delegationResolver;
};
