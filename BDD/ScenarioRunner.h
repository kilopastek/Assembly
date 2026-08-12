#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Step.h"
#include "ScenarioContext.h"

namespace bdd
{
    class ScenarioRunner
    {
    public:
        static ScenarioRunner& instance();

        void registerStep(std::unique_ptr<Step> step);

        void runStep(ScenarioContext& context, StepType type, const std::string& text) const;

    private:
        ScenarioRunner() = default;

    private:
        std::vector<std::unique_ptr<Step>> _steps;
    };
}
