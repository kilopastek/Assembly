#pragma once

#include <regex>
#include <string>
#include "Step.h"

namespace bdd
{
    using StepFunction = void (*)(ScenarioContext&, const StepArguments&);

    class FunctionStep final : public Step
    {
    public:
        FunctionStep(StepType type, std::string pattern, StepFunction function);

        StepType type() const noexcept override;

        const std::string& pattern() const noexcept override;

        std::optional<StepArguments> match(const std::string& text) const override;

        void execute(ScenarioContext& context, const StepArguments& arguments) const override;

    private:
        static std::regex compilePattern(const std::string& pattern);

        static std::string escapeRegexCharacter(char c);

    private:
        StepType _type;
        std::string _pattern;
        std::regex _regex;
        StepFunction _function;
    };
}
