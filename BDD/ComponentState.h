#include <unordered_map>
#include <string>

enum class ComponentState
{
    Stopped,
    Starting,
    Running,
    Error
};

template<typename T>
struct EnumTraits;

template<>
struct EnumTraits<ComponentState>
{
    static const std::unordered_map<std::string, ComponentState>& values()  {
        static const std::unordered_map<std::string, ComponentState> map {
                {"Stopped",  ComponentState::Stopped},
                {"Starting", ComponentState::Starting},
                {"Running",  ComponentState::Running},
                {"Error",    ComponentState::Error}
            };

        return map;
    }
};
