class ComponentFactoryRegistry
{
public:
    using Factory =
        std::function<void(ComponentBuildContext&)>;

    void registerDefault(
        const std::string& componentName,
        Factory factory)
    {
        const auto [iterator, inserted] =
            defaultFactories_.emplace(
                componentName,
                std::move(factory)
            );

        if (!inserted)
        {
            throw std::logic_error(
                "Fabrique deja enregistree : " +
                componentName
            );
        }

        constructionOrder_.push_back(componentName);
    }

    void overrideFactory(
        const std::string& componentName,
        Factory factory)
    {
        if (defaultFactories_.find(componentName) ==
            defaultFactories_.end())
        {
            throw std::logic_error(
                "Impossible de remplacer une fabrique inconnue : " +
                componentName
            );
        }

        overrides_.insert_or_assign(
            componentName,
            std::move(factory)
        );
    }

    void removeOverride(
        const std::string& componentName)
    {
        overrides_.erase(componentName);
    }

    void clearOverrides()
    {
        overrides_.clear();
    }

    void build(
        const std::string& componentName,
        ComponentBuildContext& context) const
    {
        const auto overrideIterator =
            overrides_.find(componentName);

        if (overrideIterator != overrides_.end())
        {
            overrideIterator->second(context);
            return;
        }

        const auto defaultIterator =
            defaultFactories_.find(componentName);

        if (defaultIterator == defaultFactories_.end())
        {
            throw std::logic_error(
                "Aucune fabrique pour : " +
                componentName
            );
        }

        defaultIterator->second(context);
    }

    void buildAll(ComponentBuildContext& context) const
    {
        for (const auto& componentName : constructionOrder_)
        {
            build(componentName, context);
        }
    }

private:
    std::unordered_map<std::string, Factory>
        defaultFactories_;

    std::unordered_map<std::string, Factory>
        overrides_;

    // On ne dépend pas de l'ordre non déterministe
    // d'un unordered_map.
    std::vector<std::string> constructionOrder_;
};