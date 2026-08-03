#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

// ============================================================================
// ComponentStore
//
// Possède les composants construits.
// Les composants n'ont pas besoin d'une classe de base commune.
// ============================================================================

class ComponentStore
{
private:
    class IHolder
    {
    public:
        virtual ~IHolder() = default;
    };

    template<typename Component>
    class Holder final : public IHolder
    {
    public:
        template<typename... Args>
        explicit Holder(Args&&... args)
            : component_(std::forward<Args>(args)...)
        {
        }

        Component& get() noexcept
        {
            return component_;
        }

        const Component& get() const noexcept
        {
            return component_;
        }

    private:
        Component component_;
    };

public:
    ComponentStore() = default;

    ComponentStore(const ComponentStore&) = delete;
    ComponentStore& operator=(const ComponentStore&) = delete;

    template<typename Component, typename... Args>
    Component& emplace(const std::string& name, Args&&... args)
    {
        if (components_.find(name) != components_.end())
        {
            throw std::logic_error("Composant deja construit : " + name);
        }

        auto holder = std::make_unique<Holder<Component>>(std::forward<Args>(args)...);

        Component& component = holder->get();

        components_.emplace(name, std::move(holder));

        return component;
    }

    template<typename Component>
    Component& get(const std::string& name)
    {
        const auto iterator = components_.find(name);

        if (iterator == components_.end())
        {
            throw std::logic_error("Composant inconnu : " + name);
        }

        auto* holder = dynamic_cast<Holder<Component>*>(iterator->second.get());

        if (holder == nullptr)
        {
            throw std::logic_error("Type incorrect pour le composant : " + name);
        }

        return holder->get();
    }

    template<typename Component>
    const Component& get(const std::string& name) const
    {
        const auto iterator = components_.find(name);

        if (iterator == components_.end())
        {
            throw std::logic_error("Composant inconnu : " + name);
        }

        const auto* holder = dynamic_cast<const Holder<Component>*>(iterator->second.get());

        if (holder == nullptr)
        {
            throw std::logic_error("Type incorrect pour le composant : " + name);
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

// ============================================================================
// ObserverRegistry
//
// Conserve des pointeurs non propriétaires vers les objets injectés.
// Les objets restent possédés par les composants via std::unique_ptr.
// ============================================================================

class ObserverRegistry
{
private:
    struct Key
    {
        std::string name;
        std::type_index type;

        bool operator==(const Key& other) const noexcept
        {
            return (name == other.name) && (type == other.type);
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            const auto nameHash = std::hash<std::string>{ }(key.name);

            const auto typeHash = key.type.hash_code();

            return nameHash ^ (typeHash << 1U);
        }
    };

public:
    template<typename Type>
    void expose(const std::string& name, Type& object)
    {
        const Key key{ name, typeid(Type) };

        const auto [iterator, inserted] = objects_.emplace(key, static_cast<void*>(&object));

        if (!inserted)
        {
            throw std::logic_error("Objet observable deja enregistre : " + name);
        }
    }

    template<typename Type>
    Type& get(const std::string& name)
    {
        const Key key{name, typeid(Type)};

        const auto iterator = objects_.find(key);

        if (iterator == objects_.end())
        {
            throw std::logic_error("Objet observable inconnu : " + name);
        }

        return *static_cast<Type*>(iterator->second);
    }

    template<typename Type>
    bool contains(const std::string& name) const
    {
        return objects_.find(Key{ name, typeid(Type) }) != objects_.end();
    }

    void clear()
    {
        objects_.clear();
    }

private:
    std::unordered_map<Key, void*, KeyHash> objects_;
};

// ============================================================================
// ObjectFactoryRegistry
//
// Contient les factories des objets injectés dans les composants.
//
// Chaque factory retourne un std::unique_ptr<Interface>.
// Une factory par défaut peut être remplacée avant le build.
// ============================================================================

class ObjectFactoryRegistry
{
private:
    struct Key
    {
        std::string name;
        std::type_index interfaceType;

        bool operator==(const Key& other) const noexcept
        {
            return (name == other.name) && (interfaceType == other.interfaceType);
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            const auto nameHash = std::hash<std::string>{ }(key.name);

            const auto typeHash = key.interfaceType.hash_code();

            return nameHash ^ (typeHash << 1U);
        }
    };

    class IFactory
    {
    public:
        virtual ~IFactory() = default;
    };

    template<typename Interface>
    class Factory final : public IFactory
    {
    public:
        using Function = std::function<std::unique_ptr<Interface>()>;

        explicit Factory(Function function)
            : function_(std::move(function))
        {
        }

        std::unique_ptr<Interface> create() const
        {
            return function_();
        }

    private:
        Function function_;
    };

public:
    ObjectFactoryRegistry() = default;

    ObjectFactoryRegistry(const ObjectFactoryRegistry&) = delete;
    ObjectFactoryRegistry& operator=(const ObjectFactoryRegistry&) = delete;

    template<typename Interface, typename Callable>
    void registerDefault(const std::string& name, Callable&& callable)
    {
        const Key key{ name, typeid(Interface) };

        using TypedFactory = Factory<Interface>;

        auto factory = std::make_unique<TypedFactory>(typename TypedFactory::Function(std::forward<Callable>(callable)));

        const auto [iterator, inserted] = defaults_.emplace(key, std::move(factory));

        if (!inserted)
        {
            throw std::logic_error("Factory d'objet deja enregistree : " + name);
        }
    }

    template<typename Interface, typename Callable>
    void overrideFactory(const std::string& name, Callable&& callable)
    {
        const Key key{ name, typeid(Interface) };

        if (defaults_.find(key) == defaults_.end())
        {
            throw std::logic_error("Factory par defaut inconnue : " + name);
        }

        using TypedFactory = Factory<Interface>;

        overrides_.insert_or_assign(key, std::make_unique<TypedFactory>(typename TypedFactory::Function(std::forward<Callable>(callable))));
    }

    template<typename Interface>
    std::unique_ptr<Interface> create(const std::string& name) const
    {
        const Key key{name, typeid(Interface)};

        const IFactory* selectedFactory = nullptr;

        const auto overrideIterator = overrides_.find(key);

        if (overrideIterator != overrides_.end())
        {
            selectedFactory = overrideIterator->second.get();
        }
        else
        {
            const auto defaultIterator = defaults_.find(key);

            if (defaultIterator == defaults_.end())
            {
                throw std::logic_error("Aucune factory d'objet : " + name);
            }

            selectedFactory = defaultIterator->second.get();
        }

        const auto* typedFactory = dynamic_cast<const Factory<Interface>*>(selectedFactory);

        if (typedFactory == nullptr)
        {
            throw std::logic_error("Type incorrect pour la factory : " + name);
        }

        return typedFactory->create();
    }

    void clearOverrides()
    {
        overrides_.clear();
    }

private:
    std::unordered_map<
        Key,
        std::unique_ptr<IFactory>,
        KeyHash
    > defaults_;

    std::unordered_map<
        Key,
        std::unique_ptr<IFactory>,
        KeyHash
    > overrides_;
};

// ============================================================================
// Interfaces externes au système de factory
//
// À remplacer par tes vrais registres.
// ============================================================================

class ContainerRegistry;
class ArchitectureRegistry;

// ============================================================================
// ComponentBuildContext
//
// Fourni à chaque fonction d'instanciation de composant.
// ============================================================================

struct ComponentBuildContext
{
    ContainerRegistry& containers;
    ObjectFactoryRegistry& objectFactories;
    ObserverRegistry& observers;
    ComponentStore& components;
    ArchitectureRegistry& architecture;
};

// ============================================================================
// ComponentFactoryRegistry
//
// Une factory unique par composant.
// Ces factories ne sont pas surchargeables.
// Les variations se font dans ObjectFactoryRegistry.
// ============================================================================

class ComponentFactoryRegistry
{
public:
    using Factory = std::function<void(ComponentBuildContext&)>;

    void registerFactory(const std::string& componentName, Factory factory)
    {
        const auto [iterator, inserted] = factories_.emplace(componentName, std::move(factory) );

        if (!inserted)
        {
            throw std::logic_error("Factory de composant deja enregistree : " + componentName);
        }
    }

    void buildAll(ComponentBuildContext& context) const
    {
        /*
         * L'ordre de parcours est volontairement indetermine.
         *
         * Cela suppose qu'aucun constructeur de composant ne
         * depende directement d'une autre instance de composant.
         */
        for (const auto& entry : factories_)
        {
            entry.second(context);
        }
    }

    bool contains(const std::string& componentName) const
    {
        return factories_.find(componentName) != factories_.end();
    }

private:
    std::unordered_map<
        std::string,
        Factory
    > factories_;
};

// ============================================================================
// Création d'un objet observable
//
// L'objet est retourné sous forme de unique_ptr<Interface>.
// L'ObserverRegistry conserve son adresse avec son type concret.
// ============================================================================

template<typename Interface, typename Implementation, typename... Args>
std::unique_ptr<Interface> makeObserved(ObserverRegistry& observers, const std::string& observerName, Args&&... args)
{
    static_assert(std::is_base_of_v<Interface, Implementation>, "Implementation doit heriter de Interface");

    auto object = std::make_unique<Implementation>(std::forward<Args>(args)...);

    observers.expose<Implementation>(observerName, *object);

    return object;
}