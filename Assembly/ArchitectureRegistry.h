#pragma once

class ArchitectureRegistry
{
public:
    void registerOutputs(
        const std::string& componentName,
        OutputPortRegistry& outputs)
    {
        const auto [iterator, inserted] =
            outputRegistries_.emplace(
                componentName,
                &outputs
            );

        if (!inserted)
        {
            throw std::logic_error(
                "Sorties deja enregistrees pour le composant : " +
                componentName
            );
        }
    }

    void registerInputs(
        const std::string& componentName,
        InputPortRegistry& inputs)
    {
        const auto [iterator, inserted] =
            inputRegistries_.emplace(
                componentName,
                &inputs
            );

        if (!inserted)
        {
            throw std::logic_error(
                "Entrees deja enregistrees pour le composant : " +
                componentName
            );
        }
    }

    void connect(
        const std::string& senderComponent,
        const std::string& senderPort,
        const std::string& receiverComponent,
        const std::string& receiverPort)
    {
        OutputPortRegistry& outputs =
            findOutputs(senderComponent);

        InputPortRegistry& inputs =
            findInputs(receiverComponent);

        outputs
            .port(senderPort)
            .connectTo(
                inputs.port(receiverPort)
            );
    }

private:
    OutputPortRegistry& findOutputs(
        const std::string& componentName)
    {
        const auto iterator =
            outputRegistries_.find(componentName);

        if (iterator == outputRegistries_.end())
        {
            throw std::logic_error(
                "Composant emetteur inconnu : " +
                componentName
            );
        }

        return *iterator->second;
    }

    InputPortRegistry& findInputs(
        const std::string& componentName)
    {
        const auto iterator =
            inputRegistries_.find(componentName);

        if (iterator == inputRegistries_.end())
        {
            throw std::logic_error(
                "Composant recepteur inconnu : " +
                componentName
            );
        }

        return *iterator->second;
    }

    std::unordered_map<
        std::string,
        OutputPortRegistry*
    > outputRegistries_;

    std::unordered_map<
        std::string,
        InputPortRegistry*
    > inputRegistries_;
};