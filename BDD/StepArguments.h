#pragma once

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

template<typename T>
struct EnumTraits;

namespace bdd
{
    class StepArguments
    {
    public:
        StepArguments() = default;

        explicit StepArguments(std::vector<std::string> values)
            : _values(std::move(values))
        {
        }

        std::size_t size() const noexcept
        {
            return _values.size();
        }

        const std::string& raw(std::size_t index) const
        {
            checkIndex(index);
            return _values[index];
        }

        template<typename T>
        T get(std::size_t index) const;

    private:
        void checkIndex(std::size_t index) const
        {
            if (index >= _values.size())
            {
                throw std::out_of_range("Indice de parametre BDD invalide");
            }
        }

    private:
        std::vector<std::string> _values;
    };


    template<>
    inline std::string StepArguments::get<std::string>(std::size_t index) const
    {
        checkIndex(index);
        return _values[index];
    }

    template<>
    inline int StepArguments::get<int>(std::size_t index) const
    {
        checkIndex(index);

        std::string_view text = _values[index];

        bool negative = false;

        if (!text.empty() && text.front() == '-')
        {
            negative = true;
            text.remove_prefix(1);
        }

        int base = 10;

        if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        {
            base = 16;
            text.remove_prefix(2);
        }

        unsigned int magnitude{};

        const auto result = std::from_chars(text.data(), text.data() + text.size(), magnitude, base);

        if (result.ec != std::errc{ } || result.ptr != text.data() + text.size())
        {
            throw std::invalid_argument("Paramètre entier BDD invalide");
        }

        return negative
            ? -static_cast<int>(magnitude)
            : static_cast<int>(magnitude);
    }

    template<>
    inline unsigned int StepArguments::get<unsigned int>(std::size_t index) const
    {
        checkIndex(index);

        std::string_view text = _values[index];

        int base = 10;

        if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        {
            base = 16;
            text.remove_prefix(2);
        }

        unsigned int value{};

        const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);

        if (result.ec != std::errc{ } || result.ptr != text.data() + text.size())
        {
            throw std::invalid_argument("Paramètre entier BDD invalide");
        }

        return value;
    }

    template<>
    inline double StepArguments::get<double>(std::size_t index) const
    {
        checkIndex(index);

        const auto& text = _values[index];

        char* end = nullptr;

        const double value = std::strtod(text.c_str(), &end);

        if (end == text.c_str() || *end != '\0')
        {
            throw std::invalid_argument("Parametre BDD invalide pour double : " + text);
        }

        return value;
    }

    template<typename T>
    T enumFromString(const std::string& value)
    {
        static_assert(std::is_enum_v<T>, "T doit être un enum");

        const auto& values = EnumTraits<T>::values();

        const auto it = values.find(value);

        if (it == values.end())
        {
            throw std::invalid_argument("Valeur enum BDD inconnue : " + value);
        }

        return it->second;
    }

    template<typename T>
    T StepArguments::get(std::size_t index) const
    {
        if constexpr (std::is_enum_v<T>)
        {
            return enumFromString<T>(raw(index));
        }
        else
        {
            // autres types, on utilise la spécialisation de template
        }
    }
}
