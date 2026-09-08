#pragma once

#include "Port.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CompositeInputPort class definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename Signature>
class CompositeInputPort;

template<typename... Args>
class CompositeInputPort<void(Args...)> final : public ITypedInputPort<void(Args...)>, public IDelegatingInputPort
{
public:
    using SignatureType = void(Args...);

    void delegateTo(IInputPort& input) override
    {
        auto* typedInput = dynamic_cast<ITypedInputPort<SignatureType>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error("Incompatible input delegation");
        }

        _receiver = typedInput;
    }

    void receive(Args... args) override
    {
        if (_receiver == nullptr)
        {
            throw std::logic_error("Composite input port is not delegated");
        }

        _receiver->receive(args...);
    }

private:
    ITypedInputPort<SignatureType>* _receiver = nullptr;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CompositeOutputPort class definition
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename Signature>
class CompositeOutputPort;

template<typename... Args>
class CompositeOutputPort<void(Args...)> final : public ITypedInputPort<void(Args...)>, public IOutputPort
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
        auto* typedInput = dynamic_cast<ITypedInputPort<SignatureType>*>(&input);

        if (typedInput == nullptr)
        {
            throw std::logic_error("Connexion de ports incompatibles");
        }

        _receivers.push_back(typedInput);
    }

    /*
     * Appel provenant du composant interne.
     */
    void receive(Args... args) override
    {
        send(args...);
    }

    /*
     * Redistribution vers l'extérieur du composite.
     */
    void send(Args... args)
    {
        const Message emittedMessage(args...);

        for (auto* receiver : _receivers)
        {
            Message receivedMessage = emittedMessage;

            std::apply(
                [receiver](auto&... values)
                {
                    receiver->receive(values...);
                },
                receivedMessage);
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
    std::vector<ITypedInputPort<SignatureType>*> _receivers;
};
