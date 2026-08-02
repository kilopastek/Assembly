
#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================================
// Extraction de la signature d'une méthode membre
// ============================================================================

template<typename T>
struct MemberFunctionTraits;

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionTraits<
    ReturnType (Class::*)(Args...)>
{
    using ClassType = Class;
    using Signature = ReturnType(Args...);
};

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionTraits<
    ReturnType (Class::*)(Args...) const>
{
    using ClassType = Class;
    using Signature = ReturnType(Args...);
};

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionTraits<
    ReturnType (Class::*)(Args...) noexcept>
{
    using ClassType = Class;
    using Signature = ReturnType(Args...);
};

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionTraits<
    ReturnType (Class::*)(Args...) const noexcept>
{
    using ClassType = Class;
    using Signature = ReturnType(Args...);
};

template<auto Method>
using MethodSignature =
    typename MemberFunctionTraits<
        decltype(Method)
    >::Signature;

class IPort
{
public:
    virtual ~IPort() = default;

    virtual std::type_index signature() const noexcept = 0;
};

class IInputPort : public IPort
{
public:
    ~IInputPort() override = default;
};

class IOutputPort : public IPort
{
public:
    ~IOutputPort() override = default;

    virtual void connectTo(IInputPort& input) = 0;
};

template<typename Signature>
class InputPort;

template<typename... Args>
class InputPort<void(Args...)> final : public IInputPort
{
public:
    using SignatureType = void(Args...);
    using Callback = std::function<void(Args...)>;

    template<typename Callable>
    explicit InputPort(Callable&& callback)
        : callback_(std::forward<Callable>(callback))
    {
    }

    std::type_index signature() const noexcept override
    {
        return typeid(SignatureType);
    }

    void receive(Args... args)
    {
        callback_(args...);
    }

private:
    Callback callback_;
};

template<typename Signature>
class OutputPort;

template<typename... Args>
class OutputPort<void(Args...)> final : public IOutputPort
{
public:
    using SignatureType = void(Args...);

    /*
     * Les références et les qualifications const sont retirées.
     *
     * Exemple :
     *
     * void(uint32_t, const std::vector<uint8_t>&)
     *
     * devient :
     *
     * tuple<uint32_t, std::vector<uint8_t>>
     */
    using Message =
        std::tuple<std::decay_t<Args>...>;

    std::type_index signature() const noexcept override
    {
        return typeid(SignatureType);
    }

    void connectTo(IInputPort& input) override
    {
        /*
         * Le XML garantit normalement la compatibilité.
         *
         * Le dynamic_cast constitue uniquement un garde-fou contre :
         * - un mauvais enregistrement C++ ;
         * - un mauvais nom de port ;
         * - un décalage entre XML et adaptateur de test.
         */
        auto* typedInput =
            dynamic_cast<InputPort<SignatureType>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error(
                "Connexion de ports incompatibles"
            );
        }

        receivers_.push_back(typedInput);
    }

    void send(Args... args)
    {
        /*
         * Copie représentant le message produit par l'émetteur.
         */
        const Message emittedMessage(args...);

        for (InputPort<SignatureType>* receiver : receivers_)
        {
            /*
             * Copie indépendante représentant la désérialisation
             * propre à ce récepteur.
             */
            Message receivedMessage = emittedMessage;

            std::apply(
                [receiver](auto&... values)
                {
                    receiver->receive(values...);
                },
                receivedMessage
            );
        }
    }

    std::size_t connectionCount() const noexcept
    {
        return receivers_.size();
    }

    bool isConnected() const noexcept
    {
        return !receivers_.empty();
    }

private:
    std::vector<InputPort<SignatureType>*> receivers_;
};

class OutputPortRegistry
{
public:
    void registerPort(
        const std::string& name,
        IOutputPort& port)
    {
        const auto [iterator, inserted] =
            ports_.emplace(name, &port);

        if (!inserted)
        {
            throw std::logic_error(
                "Port de sortie deja enregistre : " +
                name
            );
        }
    }

    IOutputPort& port(const std::string& name)
    {
        const auto iterator = ports_.find(name);

        if (iterator == ports_.end())
        {
            throw std::logic_error(
                "Port de sortie inconnu : " +
                name
            );
        }

        return *iterator->second;
    }

    bool contains(const std::string& name) const
    {
        return ports_.find(name) != ports_.end();
    }

private:
    std::unordered_map<
        std::string,
        IOutputPort*
    > ports_;
};

template<auto Method>
class NamedOutputPort final
{
public:
    using PortType = OutputPortFor<Method>;

    NamedOutputPort(
        OutputPortRegistry& registry,
        const std::string& name)
    {
        registry.registerPort(name, port_);
    }

    template<typename... Args>
    void send(Args&&... args)
    {
        port_.send(
            std::forward<Args>(args)...
        );
    }

    PortType& port() noexcept
    {
        return port_;
    }

    const PortType& port() const noexcept
    {
        return port_;
    }

private:
    PortType port_;
};

class InputPortRegistry
{
public:
    template<auto Method, typename Object>
    void registerPort(
        const std::string& name,
        Object& object)
    {
        using Signature = MethodSignature<Method>;

        auto inputPort =
            std::make_unique<InputPort<Signature>>(
                [&object](auto&&... args)
                {
                    std::invoke(
                        Method,
                        object,
                        std::forward<decltype(args)>(args)...
                    );
                }
            );

        IInputPort* portPointer = inputPort.get();

        const auto [iterator, inserted] =
            ports_.emplace(name, portPointer);

        if (!inserted)
        {
            throw std::logic_error(
                "Port d'entree deja enregistre : " +
                name
            );
        }

        ownedPorts_.push_back(
            std::move(inputPort)
        );
    }

    IInputPort& port(const std::string& name)
    {
        const auto iterator = ports_.find(name);

        if (iterator == ports_.end())
        {
            throw std::logic_error(
                "Port d'entree inconnu : " +
                name
            );
        }

        return *iterator->second;
    }

    bool contains(const std::string& name) const
    {
        return ports_.find(name) != ports_.end();
    }

private:
    std::unordered_map<
        std::string,
        IInputPort*
    > ports_;

    std::vector<
        std::unique_ptr<IInputPort>
    > ownedPorts_;
};

#define DECLARE_TEST_OUTPUT_PORT(Interface, PortName)           \
    NamedOutputPort<                                            \
        &Interface::PortName##_send                             \
    > PortName##_{outputs_, #PortName}


#define REGISTER_TEST_INPUT_PORT(Registry, Component, PortName) \
    Registry.registerPort<                                      \
        &std::remove_reference_t<decltype(Component)>::         \
            PortName##_receive                                  \
    >(#PortName, Component)