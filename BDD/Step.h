#pragma once

#include <optional>
#include <string>
#include "ScenarioContext.h"
#include "StepArguments.h"

namespace bdd
{
    class ScenarioContext;
    enum class StepType
    {
        Given,
        When,
        Then
    };

    class Step
    {
    public:
        virtual ~Step() = default;

        virtual StepType type() const noexcept = 0;

        virtual const std::string& pattern() const noexcept = 0;

        virtual std::optional<StepArguments> match(const std::string& text) const = 0;

        virtual void execute(ScenarioContext& context, const StepArguments& arguments) const = 0;
    };
}
