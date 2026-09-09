#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
