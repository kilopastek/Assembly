#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace test
{

    class IScheduledComponent
    {
    public:
        virtual ~IScheduledComponent() = default;

        virtual const std::string& name() const noexcept = 0;

        // Indique si le composant peut actuellement faire progresser le système.
        virtual bool hasPendingWork() const = 0;

        // Exécute une seule unité finie de traitement.
        virtual void executeOneCycle() = 0;
    };

    class TestScheduler
    {
    public:
        explicit TestScheduler(std::size_t maxCycles = 1000)
            : _maxCycles{maxCycles}
        {
        }

        void addComponent(IScheduledComponent& component)
        {
            _components.push_back(&component);
        }

        bool runOnce()
        {
            bool workExecuted = false;

            for (auto* component : _components)
            {
                if (!component->hasPendingWork())
                {
                    continue;
                }

                component->executeOneCycle();
                workExecuted = true;
            }

            return workExecuted;
        }

        std::size_t runUntilIdle()
        {
            std::size_t cycles = 0;

            while (runOnce())
            {
                ++cycles;

                if (cycles >= _maxCycles)
                {
                    throw std::runtime_error("TestScheduler: nombre maximal de cycles atteint");
                }
            }

            return cycles;
        }

        void runCycles(std::size_t count)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                runOnce();
            }
        }

        void clear()
        {
            _components.clear();
        }

    private:
        std::vector<IScheduledComponent*> _components;
        std::size_t _maxCycles;
    };
}