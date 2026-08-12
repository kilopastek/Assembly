#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>
#include <utility>
#include "ScenarioRunner.h"
#include "ScenarioContext.h"

namespace bdd
{
    namespace
    {
        const char* toString(StepType type)
        {
            switch (type)
            {
            case StepType::Given:
                return "Given";

            case StepType::When:
                return "When";

            case StepType::Then:
                return "Then";
            }

            return "Unknown";
        }
    }

    ScenarioRunner& ScenarioRunner::instance()
    {
        static ScenarioRunner instance;
        return instance;
    }

    void ScenarioRunner::registerStep(std::unique_ptr<Step> step)
    {
        if (step == nullptr)
        {
            throw std::invalid_argument("Tentative d'enregistrement d'un step nul");
        }

        _steps.push_back(std::move(step));
    }

    void ScenarioRunner::runStep(ScenarioContext& context, StepType type, const std::string& text) const
    {
        struct MatchingStep
        {
            const Step* step;
            StepArguments arguments;
        };

        std::vector<MatchingStep> matches;

        for (const auto& step : _steps)
        {
            if (step->type() != type)
            {
                continue;
            }

            auto arguments = step->match(text);

            if (arguments)
            {
                matches.push_back({ step.get(), std::move(*arguments) });
            }
        }

        if (matches.empty())
        {
            FAIL()
                << "Step BDD non implemente :\n"
                << toString(type)
                << " "
                << text;

            return;
        }

        if (matches.size() > 1)
        {
            std::ostringstream message;

            message
                << "Step BDD ambigu :\n"
                << toString(type)
                << " "
                << text
                << "\n\nPatterns correspondants :";

            for (const auto& match : matches)
            {
                message
                    << "\n  - "
                    << match.step->pattern();
            }

            FAIL() << message.str();

            return;
        }

        matches.front().step->execute(context, matches.front().arguments);
    }
}
