#include <stdexcept>
#include <utility>
#include <vector>
#include "FunctionStep.h"

namespace bdd
{
    FunctionStep::FunctionStep(StepType type, std::string pattern, StepFunction function)
        : type_{type}
        , pattern_{std::move(pattern)}
        , regex_{compilePattern(pattern_)}
        ,  function_{function}
    {
        if (function_ == nullptr)
        {
            throw std::invalid_argument("Fonction de step BDD nulle");
        }
    }

    StepType FunctionStep::type() const noexcept
    {
        return type_;
    }

    const std::string& FunctionStep::pattern() const noexcept
    {
        return pattern_;
    }

    std::optional<StepArguments> FunctionStep::match(const std::string& text) const
    {
        std::smatch matches;

        if (!std::regex_match(text, matches, regex_))
        {
            return std::nullopt;
        }

        std::vector<std::string> arguments;

        arguments.reserve(matches.size() > 0 ? matches.size() - 1 : 0);

        for (std::size_t i = 1; i < matches.size(); ++i)
        {
            arguments.push_back(matches[i].str());
        }

        return StepArguments{ std::move(arguments) };
    }

    void FunctionStep::execute(ScenarioContext& context, const StepArguments& arguments) const
    {
        function_(context, arguments);
    }

    std::regex FunctionStep::compilePattern(const std::string& pattern)
    {
        std::string expression{"^"};

        std::size_t position = 0;

        while (position < pattern.size())
        {
            if (pattern.compare(position, 5, "{int}") == 0)
            {
                expression += R"((-?(?:0[xX][0-9a-fA-F]+|[0-9]+)))";
                position += 5;
            }
            else if (pattern.compare(position, 6, "{uint}") == 0)
            {
                expression += R"((?:0[xX][0-9a-fA-F]+|[0-9]+))";
                position += 6;
            }
            else if (pattern.compare(position, 8, "{double}") == 0)
            {
                expression +=
                    R"((-?[0-9]+(?:\.[0-9]+)?))";

                position += 8;
            }
            else if (pattern.compare(position, 6, "{word}") == 0)
            {
                expression += R"((\S+))";
                position += 6;
            }
            else if (pattern.compare(position, 8, "{string}") == 0)
            {
                //expression += R"("([^"]*)")";
                expression += "\"([^\"]*)\"";
                position += 8;
            }
            else
            {
                expression += escapeRegexCharacter(pattern[position]);
                ++position;
            }
        }

        expression += "$";

        return std::regex{expression};
    }

    std::string FunctionStep::escapeRegexCharacter(char c)
    {
        switch (c)
        {
        case '.':
        case '^':
        case '$':
        case '|':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case '+':
        case '?':
        case '{':
        case '}':
        case '\\':
            return std::string{"\\"} + c;

        default:
            return std::string{1, c};
        }
    }
}
