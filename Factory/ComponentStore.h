class ComponentStore
{
private:
    class IHolder
    {
    public:
        virtual ~IHolder() = default;
        virtual std::type_index type() const noexcept = 0;
    };

    template<typename Component>
    class Holder final : public IHolder
    {
    public:
        template<typename... Args>
        explicit Holder(Args&&... args)
            : component_(
                std::forward<Args>(args)...
            )
        {
        }

        std::type_index type() const noexcept override
        {
            return typeid(Component);
        }

        Component& get() noexcept
        {
            return component_;
        }

    private:
        Component component_;
    };

public:
    template<typename Component, typename... Args>
    Component& emplace(
        const std::string& name,
        Args&&... args)
    {
        if (components_.find(name) != components_.end())
        {
            throw std::logic_error(
                "Composant deja instancie : " + name
            );
        }

        auto holder =
            std::make_unique<Holder<Component>>(
                std::forward<Args>(args)...
            );

        Component& component = holder->get();

        components_.emplace(
            name,
            std::move(holder)
        );

        return component;
    }

    template<typename Component>
    Component& get(const std::string& name)
    {
        const auto iterator = components_.find(name);

        if (iterator == components_.end())
        {
            throw std::logic_error(
                "Composant inconnu : " + name
            );
        }

        auto* holder =
            dynamic_cast<Holder<Component>*>(
                iterator->second.get()
            );

        if (holder == nullptr)
        {
            throw std::logic_error(
                "Type incorrect pour le composant : " +
                name
            );
        }

        return holder->get();
    }

    bool contains(const std::string& name) const
    {
        return components_.find(name) != components_.end();
    }

    void clear()
    {
        components_.clear();
    }

private:
    std::unordered_map<
        std::string,
        std::unique_ptr<IHolder>
    > components_;
};