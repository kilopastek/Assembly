#pragma once

#include <string>
#include <vector>

class IInputPort;
class IOutputPort;

class IInstance
{
public:
    virtual ~IInstance() = default;

    virtual const std::string& name() const = 0;

    virtual IInputPort* findInputPort(const std::string& operation) = 0;

    virtual IOutputPort* findOutputPort(const std::string& operation) = 0;

    virtual std::vector<IInputPort*> inputPorts() = 0;
    virtual std::vector<IOutputPort*> outputPorts() = 0;
};