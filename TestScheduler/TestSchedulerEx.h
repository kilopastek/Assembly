#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace test
{
    class TestScheduler
    {
    public:
        using HasWorkFunction = std::function<bool()>;
        using ExecuteFunction = std::function<void()>;

        struct Entry
        {
            std::string name;
            HasWorkFunction hasWork;
            ExecuteFunction execute;
        };

    public:
        explicit TestScheduler(std::size_t maxCycles = 1000)
            : _maxCycles(maxCycles)
        {
        }

        template<typename HasWork, typename Execute>
        void add(std::string name, HasWork&& hasWork, Execute&& execute)
        {
            _entries.push_back( Entry{
                    std::move(name),
                    std::forward<HasWork>(hasWork),
                    std::forward<Execute>(execute)
                });
        }

        bool runOnce()
        {
            bool workExecuted = false;

            for (auto& entry : _entries)
            {
                if (!entry.hasWork())
                {
                    continue;
                }

                entry.execute();
                workExecuted = true;
            }

            return workExecuted;
        }

        std::size_t runUntilIdle()
        {
            std::size_t cycles = 0LL;

            while (runOnce())
            {
                ++cycles;

                if (cycles >= _maxCycles)
                {
                    throw std::runtime_error("TestScheduler: systeme jamais idle");
                }
            }

            return cycles;
        }

        void clear()
        {
            _entries.clear();
        }

    private:
        std::vector<Entry> _entries;
        std::size_t _maxCycles;
    };
}