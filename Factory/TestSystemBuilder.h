#pragma once

#include "Factory.h"

// ============================================================================
// Description d'un lien
// ============================================================================

struct ComponentLink
{
    std::string senderComponent;
    std::string senderPort;
    std::string receiverComponent;
    std::string receiverPort;
};

// ============================================================================
// Builder du système de test
// ============================================================================

class TestSystemBuilder
{
public:
    explicit TestSystemBuilder(const ComponentDefinitionRegistry& definitions)
        : _definitions(definitions)
        , _architecture(_containers)
    {
    }

    TestSystemBuilder(const TestSystemBuilder&) = delete;
    TestSystemBuilder& operator=(const TestSystemBuilder&) = delete;

    TestSystemBuilder& addComponent(const std::string& instanceName, const std::string& componentTypeName)
    {
        ensureNotBuilt();

        /*
         * Vérification immédiate de l'existence de la définition.
         */
        _definitions.getDefinition(componentTypeName);

        const auto [iterator, inserted] = _selectedInstances.emplace(instanceName, componentTypeName);

        if (!inserted)
        {
            throw std::logic_error("Instance déjà ajoutée : " + instanceName);
        }

        return *this;
    }

    template<typename TInterface, typename TImplementation, typename... TArgs>
    TestSystemBuilder& overrideObject(const std::string& componentName, const std::string& localObjectName, TArgs&&... args)
    {
        ensureNotBuilt();

        static_assert(std::is_base_of_v<TInterface, TImplementation>, "TImplementation doit dériver de TInterface");

        const std::string objectKey = makeObjectKey(componentName, localObjectName);

        auto arguments = std::make_tuple(std::forward<TArgs>(args)...);

        _injectedObjects.overrideProvider<TInterface>(objectKey, [objectKey, observerRegistry = &_observers, arguments = std::move(arguments)]() mutable -> std::unique_ptr<TInterface> {
                return std::apply([objectKey, observerRegistry](const auto&... values) -> std::unique_ptr<TInterface> {
                        return makeObserved<TInterface, TImplementation>(*observerRegistry, objectKey, values...);
                    },
                    arguments
                );
            }
        );

        return *this;
    }

    TestSystemBuilder& loadArchitecture(const ArchitectureDescription& architecture)
    {
        ensureNotBuilt();

        for (const auto& instance : architecture.instances)
        {
            addComponentInstance(instance.instanceName, instance.componentType);
        }

        for (const auto& link : architecture.links)
        {
            defineConnection(
                link.senderInstance,
                link.senderPort,
                link.receiverInstance,
                link.receiverPort
            );
        }

        return *this;
    }

    void build()
    {
        ensureNotBuilt();

        if (_selectedInstances.empty())
        {
            throw std::logic_error("Aucun composant sélectionné");
        }

        /*
         * 1. Création des containers.
         * 2. Installation des définitions d'objets par défaut.
         */
        for (const auto& [instanceName, componentTypeName] : _selectedInstances)
        {
            const ComponentDefinition& definition = _definitions.getDefinition(componentTypeName);

            _containers.insertContainer(instanceName, definition.createContainer());

            for (const auto& objectDefinition : definition.defaultObjects)
            {
                objectDefinition.registerDefault(instanceName, _injectedObjects, _observers);
            }
        }

        /*
         * Instanciation de tous les composants.
         */
        for (const auto& [instanceName, componentTypeName] : _selectedInstances)
        {
            ComponentBuildContext context{
                instanceName,
                _containers,
                _injectedObjects,
                _observers,
                _components,
                _architecture
            };

            _definitions
                .getDefinition(componentTypeName)
                .instantiate(context);
        }

        /*
         * Connexion des composants.
         *
         * Les liens dont une extrémité n'est pas sélectionnée sont ignorés.
         */
        for (const auto& link : _links)
        {
            if (!isComponentSelected(link.senderComponent) || !isComponentSelected(link.receiverComponent))
            {
                continue;
            }

            _architecture.connect(
                link.senderComponent,
                link.senderPort,
                link.receiverComponent,
                link.receiverPort
            );
        }

        _built = true;
    }

    template<typename TComponent>
    TComponent& getComponent(const std::string& componentName)
    {
        ensureBuilt();

        return _components.getComponent<TComponent>(componentName);
    }

    template<typename TObject>
    TObject& getObject(const std::string& componentName, const std::string& localObjectName)
    {
        ensureBuilt();

        return _observers.getObject<TObject>(makeObjectKey(componentName, localObjectName));
    }

    template<typename TContainer>
    TContainer& getContainer(const std::string& componentName)
    {
        ensureBuilt();

        return _containers.getContainer<TContainer>(componentName);
    }

private:
    bool isComponentSelected(const std::string& componentName) const
    {
        return _selectedComponents.find(componentName) != _selectedComponents.end();
    }

    void ensureNotBuilt() const
    {
        if (_built)
        {
            throw std::logic_error("Le système de test est déjà construit");
        }
    }

    void ensureBuilt() const
    {
        if (!_built)
        {
            throw std::logic_error("Le système de test n'est pas construit");
        }
    }

    std::unordered_map<std::string, std::string> _selectedInstances;

    const ComponentDefinitionRegistry& _definitions;

    std::unordered_set<std::string>  _selectedComponents;

    std::vector<ComponentLink> _links;

    /*
     * Destruction en ordre inverse :
     *
     * _architecture
     * _components
     * _observers
     * _injectedObjects
     * _containers
     *
     * Les callbacks d'entrée sont donc détruits avant les composants,
     * et les composants avant les containers.
     */
    TestContainerStore _containers;
    InjectedObjectRegistry _injectedObjects;
    ObserverRegistry _observers;
    ComponentStore _components;
    ArchitectureRegistry _architecture;

    bool _built{false};
};