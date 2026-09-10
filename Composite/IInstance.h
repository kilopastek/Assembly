#pragma once
#include <string>
#include <vector>

class IInputPort;
class IOutputPort;
struct InputPortEntry
{
    std::string name;
    IInputPort* port = nullptr;
};


struct OutputPortEntry
{
    std::string name;
    IOutputPort* port = nullptr;
};


class IInstance
{
public:
    virtual ~IInstance() = default;

    virtual const std::string& name() const = 0;

    virtual IInputPort* findInputPort(const std::string& operation) = 0;

    virtual IOutputPort* findOutputPort(const std::string& operation) = 0;

    virtual std::vector<InputPortEntry> inputPorts() = 0;

    virtual std::vector<OutputPortEntry> outputPorts() = 0;
};
