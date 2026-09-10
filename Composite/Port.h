#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>


class IInputPort
{
public:
    virtual ~IInputPort() = default;

    virtual std::type_index signature() const noexcept = 0;
};


class IOutputPort
{
public:
    virtual ~IOutputPort() = default;

    virtual std::type_index signature() const noexcept = 0;

    virtual void connectTo(IInputPort& input) = 0;

    virtual std::size_t connectionCount() const noexcept = 0;

    virtual bool isConnected() const noexcept = 0;
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
        : _callback(std::forward<Callable>(callback))
    {
    }

    std::type_index signature() const noexcept override
    {
        return typeid(SignatureType);
    }

    void receive(Args... args)
    {
        _callback(args...);
    }

private:
    Callback _callback;
};


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

        /*
         * Évite de connecter deux fois exactement
         * le même port.
         */
        const auto it = std::find(_receivers.begin(), _receivers.end(), typedInput);

        if (it == _receivers.end())
        {
            _receivers.push_back(typedInput);
        }
    }

    void send(Args... args)
    {
        /*
         * Copie représentant le message produit
         * par l'émetteur.
         */
        const Message emittedMessage(args...);

        for (auto* receiver : _receivers)
        {
            /*
             * Chaque récepteur reçoit sa propre copie.
             */
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
    std::vector<InputPort<SignatureType>*> _receivers;
};
