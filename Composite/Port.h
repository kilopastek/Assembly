#pragma once

#include <functional>
#include <typeindex>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Interfaces des ports
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class IInputPort
{
public:
    virtual ~IInputPort() = default;

    virtual std::type_index signature() const noexcept = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Interface typée intermédiaire
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  

template<typename Signature>
class ITypedInputPort;

template<typename... Args>
class ITypedInputPort<void(Args...)> : public IInputPort
{
public:
    using SignatureType = void(Args...);

    std::type_index signature() const noexcept override
    {
        return typeid(SignatureType);
    }

    virtual void receive(Args... args) = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InputPort class definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename Signature>
class InputPort;

template<typename... Args>
class InputPort<void(Args...)> final : public ITypedInputPort<void(Args...)>
{
public:
    using Callback = std::function<void(Args...)>;

    template<typename Callable>
    explicit InputPort(Callable&& callback)
        : _callback(std::forward<Callable>(callback))
    {
    }

    void receive(Args... args) override
    {
        _callback(args...);
    }

private:
    Callback _callback;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IOutputPort interface definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class IOutputPort
{
public:
    virtual ~IOutputPort() = default;

    virtual std::type_index signature() const noexcept = 0;

    virtual void connectTo(IInputPort& input) = 0;

    virtual std::size_t connectionCount() const noexcept = 0;
    virtual bool isConnected() const noexcept = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OutputPort class definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename Signature>
class OutputPort;

template<typename... Args>
class OutputPort<void(Args...)> final : public IOutputPort
{
public:
    using SignatureType = void(Args...);

    using Message = std::tuple<std::decay_t<Args>...>;

    std::type_index signature() const noexcept override
    {
        return typeid(SignatureType);
    }

    void connectTo(IInputPort& input) override
    {
        auto* typedInput = dynamic_cast<InputPort<SignatureType>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error("Connexion de ports incompatibles");
        }

        _receivers.push_back(typedInput);
    }

    void send(Args... args)
    {
        const Message emittedMessage(args...);

        for (InputPort<SignatureType>* receiver : _receivers)
        {
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

    std::size_t connectionCount() const noexcept override
    {
        return _receivers.size();
    }

    bool isConnected() const noexcept override
    {
        return !_receivers.empty();
    }

private:
    std::vector<InputPort<SignatureType>*> _receivers;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IDelegatingInputPort interface definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class IDelegatingInputPort
{
public:
    virtual ~IDelegatingInputPort() = default;

    virtual void delegateTo(IInputPort& input) = 0;
};