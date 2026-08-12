#pragma once

#include <memory>
#include "FunctionStep.h"
#include "ScenarioRunner.h"

namespace bdd
{
    class StepRegistrar
    {
    public:
        StepRegistrar(StepType type, const char* pattern, StepFunction function)
        {
            ScenarioRunner::instance().registerStep(std::make_unique<FunctionStep>(type, pattern, function));
        }
    };
}
