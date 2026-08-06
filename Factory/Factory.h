#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ============================================================================
// Utilitaires
// ============================================================================

inline std::string makeObjectKey(const std::string& componentName, const std::string& objectName)
{
    return componentName + "." + objectName;
}

// ============================================================================
// Traits de fonctions membres
// ============================================================================

template<typename TMethod>
struct MemberFunctionTraits;

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunctionTraits<TReturn (TClass::*)(TArgs...)>
{
    using ClassType = TClass;
    using ReturnType = TReturn;
    using Signature = TReturn(TArgs...);
    using InputSignature = void(TArgs...);
};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunctionTraits<TReturn (TClass::*)(TArgs...) const>
{
    using ClassType = TClass;
    using ReturnType = TReturn;
    using Signature = TReturn(TArgs...);
    using InputSignature = void(TArgs...);
};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunctionTraits<TReturn (TClass::*)(TArgs...) noexcept>
{
    using ClassType = TClass;
    using ReturnType = TReturn;
    using Signature = TReturn(TArgs...);
    using InputSignature = void(TArgs...);
};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunctionTraits<TReturn (TClass::*)(TArgs...) const noexcept>
{
    using ClassType = TClass;
    using ReturnType = TReturn;
    using Signature = TReturn(TArgs...);
    using InputSignature = void(TArgs...);
};

template<auto TMethod>
using MethodSignature = typename MemberFunctionTraits<decltype(TMethod)>::Signature;

template<auto TMethod>
using MethodInputSignature = typename MemberFunctionTraits< decltype(TMethod)>::InputSignature;

// ============================================================================
// Interfaces effacées des ports
// ============================================================================

class IInputPort
{
public:
    virtual ~IInputPort() = default;
};

class IOutputPort
{
public:
    virtual ~IOutputPort() = default;

    virtual void connectTo(IInputPort& input) = 0;
};

// ============================================================================
// InputPort
// ============================================================================

template<typename TSignature>
class InputPort;

template<typename... TArgs>
class InputPort<void(TArgs...)> final : public IInputPort
{
public:
    using Callback = std::function<void(TArgs...)>;

    template<typename TCallable>
    explicit InputPort(TCallable&& callable)
        : _callback(std::forward<TCallable>(callable))
    {
    }

    void receive(TArgs... args)
    {
        _callback(args...);
    }

private:
    Callback _callback;
};

// ============================================================================
// OutputPort
//
// La fonction d'émission peut retourner un code d'état.
// Le port de réception correspondant utilise void(TArgs...).
// ============================================================================

template<typename TSignature>
class OutputPort;

// --------------------------------------------------------------------------
// Retour non void
// --------------------------------------------------------------------------

template<typename TReturn, typename... TArgs>
class OutputPort<TReturn(TArgs...)> final : public IOutputPort
{
public:
    static_assert(!std::is_void_v<TReturn>, "Cette spécialisation attend un type de retour non void");

    using InputSignature = void(TArgs...);

    /*
     * Exemple :
     *
     * TArgs... =
     *     uint32_t,
     *     const std::vector<uint8_t>&
     *
     * Message =
     *     std::tuple<
     *         uint32_t,
     *         std::vector<uint8_t>
     *     >
     */
    using Message = std::tuple<std::decay_t<TArgs>...>;

    void connectTo(IInputPort& input) override
    {
        auto* typedInput = dynamic_cast<InputPort<InputSignature>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error("Connexion entre ports incompatibles");
        }

        _receivers.push_back(typedInput);
    }

    TReturn send(TArgs... args)
    {
        /*
         * Copie représentant le message sérialisé.
         */
        const Message emittedMessage(args...);

        for (auto* receiver : _receivers)
        {
            /*
             * Copie indépendante représentant la désérialisation
             * propre au récepteur.
             */
            Message receivedMessage = emittedMessage;

            std::apply([receiver](auto&... values){
                    receiver->receive(values...);
                },
                receivedMessage
            );
        }

        return _returnValue;
    }

    void setReturnValue(TReturn value) noexcept
    {
        _returnValue = value;
    }

    TReturn getReturnValue() const noexcept
    {
        return _returnValue;
    }

    std::size_t getConnectionCount() const noexcept
    {
        return _receivers.size();
    }

    bool isConnected() const noexcept
    {
        return !_receivers.empty();
    }

private:
    std::vector<InputPort<InputSignature>*> _receivers;

    /*
     * Le FakeContainer devrait normalement configurer explicitement
     * la valeur correspondant au succès.
     */
    TReturn _returnValue{};
};

// --------------------------------------------------------------------------
// Retour void
// --------------------------------------------------------------------------

template<typename... TArgs>
class OutputPort<void(TArgs...)> final : public IOutputPort
{
public:
    using InputSignature = void(TArgs...);
    using Message = std::tuple<std::decay_t<TArgs>...>;

    void connectTo(IInputPort& input) override
    {
        auto* typedInput = dynamic_cast<InputPort<InputSignature>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error("Connexion entre ports incompatibles");
        }

        _receivers.push_back(typedInput);
    }

    void send(TArgs... args)
    {
        const Message emittedMessage(args...);

        for (auto* receiver : _receivers)
        {
            Message receivedMessage = emittedMessage;

            std::apply([receiver](auto&... values){
                    receiver->receive(values...);
                },
                receivedMessage
            );
        }
    }

    std::size_t getConnectionCount() const noexcept
    {
        return _receivers.size();
    }

    bool isConnected() const noexcept
    {
        return !_receivers.empty();
    }

private:
    std::vector<InputPort<InputSignature>*> _receivers;
};

template<auto TMethod>
using OutputPortFor = OutputPort<MethodSignature<TMethod>>;

// ============================================================================
// Registre des ports de sortie
// ============================================================================

class OutputPortRegistry
{
public:
    void registerPort(const std::string& portName, IOutputPort& port)
    {
        const auto [iterator, inserted] = _ports.emplace(portName, &port);

        if (!inserted)
        {
            throw std::logic_error("Port de sortie déjà enregistré : " + portName);
        }
    }

    IOutputPort& getPort(const std::string& portName)
    {
        const auto iterator = _ports.find(portName);

        if (iterator == _ports.end())
        {
            throw std::logic_error("Port de sortie inconnu : " + portName);
        }

        return *iterator->second;
    }

    bool containsPort(const std::string& portName) const
    {
        return _ports.find(portName) != _ports.end();
    }

private:
    std::unordered_map<std::string, IOutputPort*> _ports;
};

// ============================================================================
// Port de sortie nommé
// ============================================================================

template<auto TMethod>
class NamedOutputPort final
{
public:
    using PortType = OutputPortFor<TMethod>;

    NamedOutputPort(OutputPortRegistry& registry, const std::string& portName)
    {
        registry.registerPort(portName, _port);
    }

    template<typename... TArgs>
    decltype(auto) send(TArgs&&... args)
    {
        return _port.send(std::forward<TArgs>(args)...);
    }

    PortType& getPort() noexcept
    {
        return _port;
    }

    const PortType& getPort() const noexcept
    {
        return _port;
    }

private:
    PortType _port;
};

// ============================================================================
// Base commune des containers de test
// ============================================================================

class TestContainerBase
{
public:
    virtual ~TestContainerBase() = default;

    OutputPortRegistry& getOutputPorts() noexcept
    {
        return _outputPorts;
    }

    const OutputPortRegistry& getOutputPorts() const noexcept
    {
        return _outputPorts;
    }

private:
    OutputPortRegistry _outputPorts;
};

/*
 * Convention :
 *
 * port XML       : dataPort
 * méthode C++    : dataPort_send()
 * membre produit : _dataPort
 */
#define DEFINE_TEST_OUTPUT_PORT(TInterface, PortName)          \
    NamedOutputPort<&TInterface::PortName##_send>              \
        _##PortName{getOutputPorts(), #PortName}

// ============================================================================
// Stockage propriétaire des containers de test
// ============================================================================

class TestContainerStore
{
public:
    void insertContainer(const std::string& componentName, std::unique_ptr<TestContainerBase> container)
    {
        if (container == nullptr)
        {
            throw std::logic_error("Container nul pour le composant : " + componentName);
        }

        const auto [iterator, inserted] = _containers.emplace(componentName, std::move(container));

        if (!inserted)
        {
            throw std::logic_error("Container déjà construit : " + componentName);
        }
    }

    template<typename TContainer>
    TContainer& getContainer(const std::string& componentName)
    {
        const auto iterator = _containers.find(componentName);

        if (iterator == _containers.end())
        {
            throw std::logic_error("Container inconnu : " + componentName);
        }

        auto* typedContainer = dynamic_cast<TContainer*>(iterator->second.get());

        if (typedContainer == nullptr)
        {
            throw std::logic_error("Type de container incorrect pour : " + componentName);
        }

        return *typedContainer;
    }

    OutputPortRegistry& getOutputPorts(const std::string& componentName)
    {
        return getContainer<TestContainerBase>(componentName).getOutputPorts();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<TestContainerBase>> _containers;
};

// ============================================================================
// Registre des ports d'entrée
// ============================================================================

class InputPortRegistry
{
public:
    InputPortRegistry() = default;

    InputPortRegistry(const InputPortRegistry&) = delete;
    InputPortRegistry& operator=(const InputPortRegistry&) = delete;

    InputPortRegistry(InputPortRegistry&&) noexcept = default;
    InputPortRegistry& operator=(InputPortRegistry&&) noexcept = default;

    template<auto TMethod, typename TObject>
    void registerPort(const std::string& portName, TObject& object)
    {
        static_assert(std::is_member_function_pointer_v<decltype(TMethod)>, "TMethod doit être un pointeur vers une méthode membre");

        using Signature = MethodInputSignature<TMethod>;

        auto inputPort = std::make_unique<InputPort<Signature>>([&object](auto&&... args) {
                    std::invoke(TMethod, object, std::forward<decltype(args)>(args)...);
                }
            );

        IInputPort* portPointer = inputPort.get();

        const auto [iterator, inserted] = _ports.emplace(portName, portPointer);

        if (!inserted)
        {
            throw std::logic_error("Port d'entrée déjà enregistré : " + portName);
        }

        _ownedPorts.push_back(std::move(inputPort));
    }

    IInputPort& getPort(const std::string& portName)
    {
        const auto iterator = _ports.find(portName);

        if (iterator == _ports.end())
        {
            throw std::logic_error("Port d'entrée inconnu : " + portName);
        }

        return *iterator->second;
    }

private:
    std::unordered_map<std::string, IInputPort*> _ports;

    std::vector<std::unique_ptr<IInputPort>> _ownedPorts;
};

// ============================================================================
// Registre d'architecture
//
// Possède les InputPortRegistry.
// Les OutputPortRegistry appartiennent aux containers.
// ============================================================================

class ArchitectureRegistry
{
public:
    explicit ArchitectureRegistry(TestContainerStore& containers)
        : _containers(containers)
    {
    }

    template<auto TMethod, typename TComponent>
    void registerInput(const std::string& componentName, const std::string& portName, TComponent& component)
    {
        _inputRegistries[componentName].template registerPort<TMethod>(portName, component);
    }

    void connect(
        const std::string& senderComponent,
        const std::string& senderPort,
        const std::string& receiverComponent,
        const std::string& receiverPort)
    {
        IOutputPort& output = _containers.getOutputPorts(senderComponent)
                                         .getPort(senderPort);

        IInputPort& input = getInputRegistry(receiverComponent).getPort(receiverPort);

        output.connectTo(input);
    }

private:
    InputPortRegistry& getInputRegistry(const std::string& componentName)
    {
        const auto iterator = _inputRegistries.find(componentName);

        if (iterator == _inputRegistries.end())
        {
            throw std::logic_error("Aucun port d'entrée enregistré pour : " + componentName);
        }

        return iterator->second;
    }

    TestContainerStore& _containers;

    std::unordered_map<std::string, InputPortRegistry> _inputRegistries;
};

// ============================================================================
// Stockage propriétaire des composants
// ============================================================================

class ComponentStore
{
private:
    class IComponentInstance
    {
    public:
        virtual ~IComponentInstance() = default;
    };

    template<typename TComponent>
    class ComponentInstance final : public IComponentInstance
    {
    public:
        template<typename... TArgs>
        explicit ComponentInstance(TArgs&&... args)
            : _component(std::forward<TArgs>(args)...)
        {
        }

        TComponent& getComponent() noexcept
        {
            return _component;
        }

    private:
        TComponent _component;
    };

public:
    template<typename TComponent, typename... TArgs>
    TComponent& emplaceComponent(const std::string& componentName, TArgs&&... args)
    {
        if (_components.find(componentName) != _components.end())
        {
            throw std::logic_error("Composant déjà construit : " + componentName);
        }

        auto instance = std::make_unique<ComponentInstance<TComponent>>(std::forward<TArgs>(args)...);

        TComponent& component = instance->getComponent();

        _components.emplace(componentName, std::move(instance));

        return component;
    }

    template<typename TComponent>
    TComponent& getComponent(
        const std::string& componentName)
    {
        const auto iterator = _components.find(componentName);

        if (iterator == _components.end())
        {
            throw std::logic_error("Composant inconnu : " + componentName);
        }

        auto* instance = dynamic_cast<ComponentInstance<TComponent>*>(iterator->second.get());

        if (instance == nullptr)
        {
            throw std::logic_error("Type incorrect pour le composant : " + componentName);
        }

        return instance->getComponent();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IComponentInstance>> _components;
};

// ============================================================================
// Registre des objets observables
//
// Ne possède pas les objets.
// ============================================================================

class ObserverRegistry
{
private:
    struct Key
    {
        std::string name;
        std::type_index type;

        bool operator==(const Key& other) const noexcept
        {
            return (name == other.name) && (type == other.type);
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            const auto nameHash = std::hash<std::string>{}(key.name);
            const auto typeHash = key.type.hash_code();

            return nameHash ^ (typeHash << 1U);
        }
    };

public:
    template<typename TObject>
    void exposeObject(const std::string& objectName, TObject& object)
    {
        const Key key{ objectName, typeid(TObject) };

        const auto [iterator, inserted] = _objects.emplace(key, static_cast<void*>(&object));

        if (!inserted)
        {
            throw std::logic_error("Objet observable déjà enregistré : " + objectName));
        }
    }

    template<typename TObject>
    TObject& getObject(const std::string& objectName)
    {
        const Key key{ objectName, typeid(TObject) };

        const auto iterator = _objects.find(key);

        if (iterator == _objects.end())
        {
            throw std::logic_error("Objet observable inconnu : " + objectName);
        }

        return *static_cast<TObject*>(iterator->second);
    }

private:
    std::unordered_map<Key, void*, KeyHash> _objects;
};

// ============================================================================
// Création d'un objet observé
// ============================================================================

template<typename TInterface, typename TImplementation, typename... TArgs>
std::unique_ptr<TInterface> makeObserved(ObserverRegistry& observers, const std::string& objectName, TArgs&&... args)
{
    static_assert(std::is_base_of_v<TInterface, TImplementation>, "TImplementation doit dériver de TInterface");

    auto object = std::make_unique<TImplementation>(std::forward<TArgs>(args)...);

    observers.exposeObject<TImplementation>(objectName, *object);

    return object;
}

// ============================================================================
// Registre des objets injectés
// ============================================================================

class InjectedObjectRegistry
{
private:
    struct Key
    {
        std::string name;
        std::type_index interfaceType;

        bool operator==(const Key& other) const noexcept
        {
            return (name == other.name) && (interfaceType == other.interfaceType);
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            const auto nameHash = std::hash<std::string>{}(key.name);
            const auto typeHash = key.interfaceType.hash_code();

            return nameHash ^ (typeHash << 1U);
        }
    };

    class IProvider
    {
    public:
        virtual ~IProvider() = default;
    };

    template<typename TInterface>
    class Provider final : public IProvider
    {
    public:
        using Factory = std::function<std::unique_ptr<TInterface>()>;

        explicit Provider(Factory factory)
            : _factory(std::move(factory))
        {
        }

        std::unique_ptr<TInterface> createObject() const
        {
            return _factory();
        }

    private:
        Factory _factory;
    };

public:
    template<typename TInterface, typename TCallable>
    void registerDefault(const std::string& objectName, TCallable&& callable)
    {
        const Key key{ objectName, typeid(TInterface) };

        const auto [iterator, inserted] = _defaultProviders.emplace(key, 
                                                                    std::make_unique<Provider<TInterface>>(std::forward<TCallable>(callable)));

        if (!inserted)
        {
            throw std::logic_error("Objet injectable par défaut déjà défini : " + objectName);
        }
    }

    template<typename TInterface, typename TCallable>
    void overrideProvider(const std::string& objectName, TCallable&& callable)
    {
        const Key key{ objectName, typeid(TInterface) };

        _overriddenProviders.insert_or_assign(key, std::make_unique<Provider<TInterface>>(std::forward<TCallable>(callable)));
    }

    template<typename TInterface>
    std::unique_ptr<TInterface> createObject(const std::string& objectName) const
    {
        const Key key{objectName, typeid(TInterface)};

        const IProvider* provider = nullptr;

        const auto overriddenIterator = _overriddenProviders.find(key);

        if (overriddenIterator != _overriddenProviders.end())
        {
            provider = overriddenIterator->second.get();
        }
        else
        {
            const auto defaultIterator = _defaultProviders.find(key);

            if (defaultIterator == _defaultProviders.end())
            {
                throw std::logic_error("Objet injectable inconnu : " + objectName);
            }

            provider = defaultIterator->second.get();
        }

        const auto* typedProvider = dynamic_cast<const Provider<TInterface>*>(provider);

        if (typedProvider == nullptr)
        {
            throw std::logic_error("Interface incorrecte pour l'objet : " + objectName);
        }

        return typedProvider->createObject();
    }

private:
    std::unordered_map<
        Key,
        std::unique_ptr<IProvider>,
        KeyHash
    > _defaultProviders;

    std::unordered_map<
        Key,
        std::unique_ptr<IProvider>,
        KeyHash
    > _overriddenProviders;
};

// ============================================================================
// Contexte fourni à la fonction d'instanciation
// ============================================================================

struct ComponentBuildContext
{
    const std::string& instanceName;
    TestContainerStore& containers;
    InjectedObjectRegistry& injectedObjects;
    ObserverRegistry& observers;
    ComponentStore& components;
    ArchitectureRegistry& architecture;

    std::string makeObjectKey(const std::string& localObjectName) const
    {
        return instanceName + "." + localObjectName;
    }
};

// ============================================================================
// Définition effacée d'un objet injecté par défaut
// ============================================================================

struct InjectedObjectDefinition
{
    using RegistrationFunction = std::function<void(const std::string& instanceName, InjectedObjectRegistry&, ObserverRegistry&)>;

    RegistrationFunction registerDefault;
};

// ============================================================================
// Construction d'une définition d'objet injecté
//
// Les arguments stockés doivent être copiables pour que la définition puisse
// être réutilisée par plusieurs builders.
// ============================================================================

template<typename TInterface, typename TImplementation, typename... TArgs>
InjectedObjectDefinition makeInjectedObjectDefinition(std::string localObjectName,TArgs&&... args)
{
    static_assert(std::is_base_of_v<TInterface, TImplementation>, "TImplementation doit dériver de TInterface");

    using StoredArguments = std::tuple<std::decay_t<TArgs>...>;

    StoredArguments storedArguments(std::forward<TArgs>(args)...);

    return InjectedObjectDefinition{
        [
            localObjectName = std::move(localObjectName),
            storedArguments = std::move(storedArguments)
        ](const std::string& instanceName, InjectedObjectRegistry& objects, ObserverRegistry& observers) {
            const std::string objectKey = instanceName + "." + localObjectName;

            auto argumentsForInstance = storedArguments;

            objects.registerDefault<TInterface>(
                objectKey, 
                [objectKey, observerRegistry = &observers,  arguments = std::move(argumentsForInstance)]() mutable -> std::unique_ptr<TInterface> {
                    return std::apply(
                        [objectKey, observerRegistry](const auto&... values) -> std::unique_ptr<TInterface> {
                            return makeObserved<TInterface, TImplementation>(*observerRegistry, objectKey, values...);
                        },
                        arguments
                    );
                }
            );
        }
    };
}

// ============================================================================
// Définition autonome d'un composant
// ============================================================================

struct ComponentDefinition
{
    using ContainerFactory = std::function<std::unique_ptr<TestContainerBase>()>;

    using InstantiateFunction = std::function<void(ComponentBuildContext&)>;

    ContainerFactory createContainer;

    std::vector<InjectedObjectDefinition> defaultObjects;

    InstantiateFunction instantiate;
};

// ============================================================================
// Builder de définition d'un composant
// ============================================================================


class ComponentDefinitionBuilder
{
public:
    explicit ComponentDefinitionBuilder(std::string componentTypeName)
        : _componentTypeName(std::move(componentTypeName))
    {
    }

    template<typename TContainer>
    ComponentDefinitionBuilder& defineContainer()
    {
        static_assert(std::is_base_of_v<TestContainerBase, TContainer>, "TContainer doit dériver de TestContainerBase");

        _definition.createContainer = []() -> std::unique_ptr<TestContainerBase> {
                return std::make_unique<TContainer>();
            };

        return *this;
    }

    template<typename TInterface, typename TImplementation, typename... TArgs>
    ComponentDefinitionBuilder& defineObject(const std::string& localObjectName, TArgs&&... args)
    {
        _definition.defaultObjects.push_back(
            makeInjectedObjectDefinition<TInterface, TImplementation >(localObjectName, std::forward<TArgs>(args)...)
        );

        return *this;
    }

    template<typename TCallable>
    ComponentDefinitionBuilder& defineInstantiation(TCallable&& callable)
    {
        _definition.instantiate = std::forward<TCallable>(callable);

        return *this;
    }

    ComponentDefinition build()
    {
        if (!_definition.createContainer)
        {
            throw std::logic_error("Container non défini pour le type : " + _componentTypeName);
        }

        if (!_definition.instantiate)
        {
            throw std::logic_error("Instanciation non définie pour le type : " + _componentTypeName);
        }

        return std::move(_definition);
    }

private:
    std::string _componentTypeName;
    ComponentDefinition _definition;
};

// ============================================================================
// Catalogue des définitions de composants
// ============================================================================

class ComponentDefinitionRegistry
{
public:
    void registerDefinition(const std::string& componentName, ComponentDefinition definition)
    {
        const auto [iterator, inserted] = _definitions.emplace(componentName, std::move(definition));

        if (!inserted)
        {
            throw std::logic_error("Définition déjà enregistrée : " + componentName);
        }
    }

    const ComponentDefinition& getDefinition(const std::string& componentName) const
    {
        const auto iterator = _definitions.find(componentName);

        if (iterator == _definitions.end())
        {
            throw std::logic_error("Composant non référencé : " + componentName);
        }

        return iterator->second;
    }

private:
    std::unordered_map<std::string, ComponentDefinition> _definitions;
};

