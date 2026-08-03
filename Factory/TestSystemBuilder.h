class TestSystemBuilder
{
public:
    TestSystemBuilder(
        ContainerRegistry& containers,
        ArchitectureRegistry& architecture,
        const ComponentFactoryRegistry& componentFactories)
        : containers_(containers),
          architecture_(architecture),
          componentFactories_(componentFactories)
    {
    }

    TestSystemBuilder(const TestSystemBuilder&) = delete;
    TestSystemBuilder& operator=(const TestSystemBuilder&) = delete;

    template<
        typename Interface,
        typename Implementation,
        typename... Args
    >
    void registerDefaultObject(
        const std::string& objectName,
        Args&&... args)
    {
        static_assert(
            std::is_base_of_v<Interface, Implementation>,
            "Implementation doit heriter de Interface"
        );

        /*
         * Les arguments sont stockes par valeur dans le tuple.
         * La factory pourra être appelée plus tard pendant build().
         */
        auto constructorArguments =
            std::make_tuple(
                std::forward<Args>(args)...
            );

        objectFactories_.registerDefault<Interface>(
            objectName,
            [this,
             objectName,
             constructorArguments =
                 std::move(constructorArguments)]() mutable
                -> std::unique_ptr<Interface>
            {
                return std::apply(
                    [this, &objectName](auto&&... values)
                        -> std::unique_ptr<Interface>
                    {
                        return makeObserved<
                            Interface,
                            Implementation
                        >(
                            observers_,
                            objectName,
                            std::forward<
                                decltype(values)
                            >(values)...
                        );
                    },
                    constructorArguments
                );
            }
        );
    }

    template<
        typename Interface,
        typename Implementation,
        typename... Args
    >
    void overrideObject(
        const std::string& objectName,
        Args&&... args)
    {
        static_assert(
            std::is_base_of_v<Interface, Implementation>,
            "Implementation doit heriter de Interface"
        );

        auto constructorArguments =
            std::make_tuple(
                std::forward<Args>(args)...
            );

        objectFactories_.overrideFactory<Interface>(
            objectName,
            [this,
             objectName,
             constructorArguments =
                 std::move(constructorArguments)]() mutable
                -> std::unique_ptr<Interface>
            {
                return std::apply(
                    [this, &objectName](auto&&... values)
                        -> std::unique_ptr<Interface>
                    {
                        return makeObserved<
                            Interface,
                            Implementation
                        >(
                            observers_,
                            objectName,
                            std::forward<
                                decltype(values)
                            >(values)...
                        );
                    },
                    constructorArguments
                );
            }
        );
    }

    void build()
    {
        if (built_)
        {
            throw std::logic_error(
                "Le systeme de test est deja construit"
            );
        }

        ComponentBuildContext context{
            containers_,
            objectFactories_,
            observers_,
            components_,
            architecture_
        };

        componentFactories_.buildAll(context);

        /*
         * Les connexions doivent être construites après tous
         * les composants et tous les ports d'entrée.
         */
        connectArchitecture();

        built_ = true;
    }

    template<typename Component>
    Component& component(const std::string& name)
    {
        ensureBuilt();

        return components_.get<Component>(name);
    }

    template<typename Mock>
    Mock& mock(const std::string& name)
    {
        ensureBuilt();

        return observers_.get<Mock>(name);
    }

    ObjectFactoryRegistry& objectFactories() noexcept
    {
        return objectFactories_;
    }

    ObserverRegistry& observers() noexcept
    {
        return observers_;
    }

    ComponentStore& components() noexcept
    {
        return components_;
    }

private:
    void ensureBuilt() const
    {
        if (!built_)
        {
            throw std::logic_error(
                "Le systeme de test n'est pas construit"
            );
        }
    }

    void connectArchitecture()
    {
        /*
         * À remplacer par :
         *
         * - les connexions codées en dur ;
         * - ou le chargement des liens XML.
         *
         * Exemple :
         *
         * architecture_.connect(
         *     "ComponentA",
         *     "dataPort",
         *     "ComponentB",
         *     "dataPort"
         * );
         */
    }

    ContainerRegistry& containers_;
    ArchitectureRegistry& architecture_;

    const ComponentFactoryRegistry& componentFactories_;

    ObjectFactoryRegistry objectFactories_;
    ObserverRegistry observers_;
    ComponentStore components_;

    bool built_{false};
};
