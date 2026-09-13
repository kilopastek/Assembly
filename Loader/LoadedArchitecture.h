#pragma once

#include <pugixml.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace architecture
{

/*
 * ============================================================================
 * Résultat public du loader
 * ============================================================================
 */

struct ComponentInstance
{
    std::string name;
    std::string componentType;
    std::string implementation;
};

struct Connection
{
    std::string senderInstance;
    std::string senderOperation;

    std::string receiverInstance;
    std::string receiverOperation;
};

struct LoadedArchitecture
{
    std::vector<ComponentInstance> instances;
    std::vector<Connection> connections;
};

/*
 * ============================================================================
 * Interface d'un composant
 * ============================================================================
 *
 * La direction n'est pas utilisée pour valider les eventLink explicites.
 *
 * Elle est uniquement utile pour :
 *
 *   - générer les implicitLinks ;
 *   - inverser les ports du shadow.
 */

enum class OperationDirection
{
    Sent,
    Received
};

struct Operation
{
    std::string name;
    OperationDirection direction;
};

/*
 * ============================================================================
 * Endpoint réel
 * ============================================================================
 */

struct Endpoint
{
    std::string instance;
    std::string operation;

    bool operator==(const Endpoint& other) const noexcept
    {
        return (instance == other.instance) && (operation == other.operation);
    }
};

using EndpointList = std::vector<Endpoint>;

/*
 * ============================================================================
 * Description XML d'un lien
 * ============================================================================
 */

struct LinkEndpoint
{
    std::string instance;
    std::string operation;
};

struct LinkDescription
{
    LinkEndpoint sender;
    LinkEndpoint receiver;
};

/*
 * ============================================================================
 * Description XML d'une instance
 * ============================================================================
 */

struct InstanceDescription
{
    std::string name;
    std::string componentType;
    std::string implementation;

    bool isComposite() const noexcept
    {
        return implementation == "composite";
    }
};

/*
 * ============================================================================
 * Description d'un implicitLinks
 * ============================================================================
 */

struct ImplicitLinkRule
{
    std::string instance = "*";
    std::string prefix;
};

/*
 * ============================================================================
 * Assembly parsé depuis XML
 * ============================================================================
 */

struct AssemblyDescription
{
    std::vector<InstanceDescription> instances;
    std::vector<LinkDescription> explicitLinks;
    std::vector<ImplicitLinkRule> implicitLinks;
};

/*
 * ============================================================================
 * Localisation d'un composant sur disque
 * ============================================================================
 */

struct ComponentLocation
{
    std::string componentType;

    std::filesystem::path projectDirectory;
    std::filesystem::path componentFile;
    std::filesystem::path assemblyFile;

    bool hasAssembly() const
    {
        return (!assemblyFile.empty()) && (std::filesystem::exists(assemblyFile));
    }
};

/*
 * ============================================================================
 * Catalogue des mini-projets
 * ============================================================================
 */

class ProjectCatalog
{
public:

    void scan(const std::filesystem::path& componentProjects, const std::filesystem::path& moduleProjects)
    {
        _projects.clear();
        _globalIndex.clear();

        scanProjectsDirectory(componentProjects);
        scanProjectsDirectory(moduleProjects);
    }

    const ComponentLocation& findLocal(const std::filesystem::path& projectDirectory, const std::string& componentType) const
    {
        const std::string key = normalizedString(projectDirectory);

        const auto projectIt = _projects.find(key);

        if (projectIt == _projects.end())
        {
            throw std::logic_error("Mini-projet inconnu : " + projectDirectory.string());
        }

        const auto componentIt = projectIt->second.find(componentType);

        if (componentIt == projectIt->second.end())
        {
            throw std::logic_error("Composant '" + componentType + "' introuvable dans le mini-projet '" + projectDirectory.string() + "'");
        }

        return componentIt->second;
    }

    const ComponentLocation& findGlobal(const std::string& componentType) const
    {
        const ComponentLocation* found = nullptr;

        for (const auto& [_, components] : _projects)
        {
            const auto it = components.find(componentType);

            if (it == components.end())
            {
                continue;
            }

            if (found != nullptr)
            {
                throw std::logic_error("Composant ambigu : " + componentType);
            }

            found = &it->second;
        }

        if (found == nullptr)
        {
            throw std::logic_error("Composant introuvable : " + componentType);
        }

        return *found;
    }

private:

    using ComponentMap = std::unordered_map<std::string, ComponentLocation>;

    void scanProjectsDirectory(const std::filesystem::path& projectsDirectory)
    {
        if (!std::filesystem::exists(projectsDirectory))
        {
            return;
        }

        /*
         * Chaque sous-dossier immédiat représente
         * un mini-projet.
         */
        for (const auto& entry : std::filesystem::directory_iterator(projectsDirectory))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            scanMiniProject(entry.path());
        }
    }

    void scanMiniProject(const std::filesystem::path& projectDirectory)
    {
        ComponentMap components;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDirectory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const auto& path = entry.path();
            const std::string filename = path.filename().string();
            constexpr const char* suffix = ".comp.xml";

            if (!endsWith(filename, suffix))
            {
                continue;
            }

            std::string componentType = filename.substr(0, filename.size() - std::string(suffix).size());

            ComponentLocation location;
            location.componentType    = componentType;
            location.projectDirectory = projectDirectory;
            location.componentFile    = path;
            location.assemblyFile     = path.parent_path() / "composite" / (componentType + ".composite.assembly.xml");

            const auto [iterator, inserted] = components.emplace(componentType, std::move(location));

            if (!inserted)
            {
                throw std::logic_error("Le composant '" + componentType + "' est defini plusieurs fois dans '" + projectDirectory.string() + "'");
            }
        }

        const std::string projectKey = normalizedString(projectDirectory);

        const auto [iterator, inserted] = _projects.emplace(projectKey, std::move(components));

        if (!inserted)
        {
            throw std::logic_error("Mini-projet duplique : " + projectDirectory.string());
        }
    }

    static bool endsWith(const std::string& value, const std::string& suffix)
    {
        if (value.size() < suffix.size())
        {
            return false;
        }

        return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    static std::string normalizedString(const std::filesystem::path& path)
    {
        return std::filesystem::absolute(path).lexically_normal().string();
    }

private:

    std::unordered_map<std::string, ComponentMap> _projects;
};

/*
 * ============================================================================
 * Parser du fichier .comp.xml
 * ============================================================================
 */

class ComponentFileParser
{
public:

    std::vector<Operation> parse(const std::filesystem::path& file) const
    {
        pugi::xml_document document;

        const auto result = document.load_file(file.string().c_str());

        if (!result)
        {
            throw std::runtime_error("Impossible de parser '" + file.string() + "' : " + result.description());
        }

        std::vector<Operation> operations;

        collectOperations(document.document_element(), operations);

        return operations;
    }

private:

    void collectOperations(const pugi::xml_node& node, std::vector<Operation>& operations) const
    {
        const std::string nodeName = node.name();

        if (nodeName == "eventSent" || nodeName == "eventSender")
        {
            operations.push_back({ readOperationName(node), OperationDirection::Sent });
        }
        else if (nodeName == "eventReceive" || nodeName == "eventReceiver")
        {
            operations.push_back({ readOperationName(node), OperationDirection::Received });
        }

        for (const auto& child : node.children())
        {
            collectOperations(child, operations);
        }
    }

    static std::string readOperationName(const pugi::xml_node& node)
    {
        /*
         * Ajuster ici uniquement si le XML réel
         * stocke le nom différemment.
         */

        std::string name = node.attribute("name").as_string();

        if (!name.empty())
        {
            return name;
        }

        name = node.attribute("operation").as_string();

        if (!name.empty())
        {
            return name;
        }

        if (const auto nameNode = node.child("name"))
        {
            name = nameNode.text().as_string();
        }

        if (name.empty())
        {
            throw std::logic_error(std::string( "Operation sans nom dans <") + node.name() + ">");
        }

        return name;
    }
};

/*
 * ============================================================================
 * Parser du fichier .composite.assembly.xml
 * ============================================================================
 */

class AssemblyFileParser
{
public:

    AssemblyDescription parse(const std::filesystem::path& file) const
    {
        pugi::xml_document document;

        const auto result = document.load_file(file.string().c_str());

        if (!result)
        {
            throw std::runtime_error("Impossible de parser '" + file.string() + "' : " + result.description());
        }

        AssemblyDescription description;

        collect(document.document_element(), false, description);

        return description;
    }

private:

    void collect(const pugi::xml_node& node, bool insideImplicitLinks, AssemblyDescription& description) const
    {
        const std::string nodeName = node.name();

        if (nodeName == "instance")
        {
            parseInstance(node, description);
        }
        else if (nodeName == "eventLink")
        {
            parseEventLink(node, description);
        }
        else if (insideImplicitLinks && nodeName == "operations")
        {
            parseImplicitRule(node, description);
        }

        const bool childInsideImplicit = insideImplicitLinks || nodeName == "implicitLinks";

        for (const auto& child : node.children())
        {
            collect(child, childInsideImplicit, description);
        }
    }

    static void parseInstance(const pugi::xml_node& node, AssemblyDescription& description)
    {
        InstanceDescription instance;

        instance.name           = node.attribute("name").as_string();
        instance.componentType  = node.attribute("componentType").as_string();
        instance.implementation = node.attribute("implementation").as_string();

        if (instance.name.empty() || instance.componentType.empty())
        {
            throw std::logic_error("Declaration d'instance incomplete");
        }

        description.instances.push_back(std::move(instance));
    }

    static void parseEventLink(const pugi::xml_node& node, AssemblyDescription& description)
    {
        const auto sender = node.child("sender");
        const auto receiver = node.child("receiver");

        if (!sender || !receiver)
        {
            throw std::logic_error("eventLink incomplet");
        }

        LinkDescription link;

        link.sender.instance    = sender.attribute("instanceName").as_string();
        link.sender.operation   = sender.attribute("operation").as_string();
        link.receiver.instance  = receiver.attribute("instanceName").as_string();
        link.receiver.operation = receiver.attribute("operation").as_string();

        if (link.sender.instance.empty() ||
            link.sender.operation.empty() ||
            link.receiver.instance.empty() ||
            link.receiver.operation.empty())
        {
            throw std::logic_error("eventLink incomplet");
        }

        description.explicitLinks.push_back(std::move(link));
    }

    static void parseImplicitRule(const pugi::xml_node& node, AssemblyDescription& description)
    {
        ImplicitLinkRule rule;

        rule.instance = node.attribute("instance").as_string("*");
        rule.prefix = node.attribute("prefix").as_string("");

        description.implicitLinks.push_back(std::move(rule));
    }
};

/*
 * ============================================================================
 * Composant et instance internes au loader
 * ============================================================================
 */

struct Assembly;

struct Component
{
    std::string componentType;

    std::vector<Operation> operations;

    /*
     * nullptr pour un composant terminal.
     */
    std::unique_ptr<Assembly> assembly;

    bool isComposite() const noexcept
    {
        return assembly != nullptr;
    }
};

struct Instance
{
    std::string name;
    std::string componentType;
    std::string implementation;

    bool shadow = false;

    /*
     * Interface logique de l'instance.
     */
    std::vector<Operation> operations;

    /*
     * operation logique
     *       ->
     * endpoints réels
     */
    std::unordered_map<std::string, EndpointList> bindings;

    EndpointList endpoints(const std::string& operation) const
    {
        const auto it = bindings.find(operation);

        if (it == bindings.end())
        {
            throw std::logic_error("Operation '" + name + "." + operation + "' non bindee");
        }

        return it->second;
    }

    bool hasOperation(const std::string& operation) const
    {
        return
            std::any_of(
                operations.begin(),
                operations.end(),
                [&](const Operation& current)
                {
                    return current.name == operation;
                });
    }

    bool hasOperation(const std::string& operation, OperationDirection direction) const
    {
        return
            std::any_of(
                operations.begin(),
                operations.end(),
                [&](const Operation& current)
                {
                    return (current.name == operation) && (current.direction == direction);
                });
    }
};

/*
 * ============================================================================
 * Assembly runtime temporaire
 * ============================================================================
 */

struct Assembly
{
    static constexpr const char* ShadowInstanceName = "$shadow";

    void addInstance(Instance instance)
    {
        const std::string name = instance.name;

        const auto result = _instances.emplace(name, std::move(instance));

        if (!result.second)
        {
            throw std::logic_error("Instance locale dupliquee : " + name);
        }
    }

    Instance& instance(const std::string& name)
    {
        const auto it = _instances.find(name);

        if (it == _instances.end())
        {
            throw std::logic_error("Instance '" + name + "' inconnue");
        }

        return it->second;
    }

    const Instance& instance(const std::string& name) const
    {
        const auto it = _instances.find(name);

        if (it == _instances.end())
        {
            throw std::logic_error("Instance '" + name + "' inconnue");
        }

        return it->second;
    }

    Instance& shadow()
    {
        return instance(ShadowInstanceName);
    }

    const Instance& shadow() const
    {
        return instance(ShadowInstanceName);
    }

    const auto& instances() const
    {
        return _instances;
    }

private:

    std::unordered_map<std::string, Instance> _instances;
};

/*
 * ============================================================================
 * Ports utilisés explicitement
 * ============================================================================
 */

struct LogicalEndpoint
{
    std::string instance;
    std::string operation;

    bool operator<(const LogicalEndpoint& other) const noexcept
    {
        if (instance != other.instance)
        {
            return instance < other.instance;
        }

        return operation < other.operation;
    }
};

class ExplicitConnectionRegistry
{
public:

    void add(const LinkDescription& link)
    {
        _senders.insert({ link.sender.instance, link.sender.operation });
        _receivers.insert({ link.receiver.instance, link.receiver.operation });
    }

    bool senderUsed(const std::string& instance, const std::string& operation) const
    {
        return
            _senders.find({ instance, operation }) != _senders.end();
    }

    bool receiverUsed(const std::string& instance, const std::string& operation) const
    {
        return
            _receivers.find({ instance, operation }) != _receivers.end();
    }

private:

    std::set<LogicalEndpoint> _senders;
    std::set<LogicalEndpoint> _receivers;
};

/*
 * ============================================================================
 * ArchitectureLoader
 * ============================================================================
 */

class ArchitectureLoader
{
public:
    ArchitectureLoader(std::filesystem::path componentProjectsDirectory, std::filesystem::path moduleProjectsDirectory)
        : _componentProjectsDirectory(std::move(componentProjectsDirectory))
        , _moduleProjectsDirectory(std::move(moduleProjectsDirectory))
    {
    }

    LoadedArchitecture load(const std::string& rootComponentType)
    {
        _architecture = { };

        _instanceNames.clear();
        _connectionKeys.clear();

        _catalog.scan(_componentProjectsDirectory, _moduleProjectsDirectory);

        const ComponentLocation& rootLocation = _catalog.findGlobal(rootComponentType);

        /*
         * Le root est parsé exactement comme n'importe
         * quel autre composant.
         *
         * Le nom "__root__" n'apparaitra jamais
         * dans l'architecture finale.
         */
        parseComponent(rootLocation, "__root__");

        return _architecture;
    }

private:
    /*
     * ========================================================================
     * Parsing récursif d'un composant
     * ========================================================================
     */

    Component parseComponent(const ComponentLocation& location, const std::string& instanceName)
    {
        Component component;

        component.componentType = location.componentType;
        component.operations    = _componentParser.parse(location.componentFile);

        if (location.hasAssembly())
        {
            component.assembly = parseAssembly(location, instanceName, component.operations);
        }

        return component;
    }

    /*
     * ========================================================================
     * Construction d'un assembly
     * ========================================================================
     */

    std::unique_ptr<Assembly> parseAssembly(const ComponentLocation& location, const std::string& ownerInstanceName, const std::vector<Operation>& ownerOperations)
    {
        auto assembly = std::make_unique<Assembly>();

        /*
         * 1. Création du shadow.
         */
        assembly->addInstance(createShadow(ownerOperations));

        const AssemblyDescription description = _assemblyParser.parse(location.assemblyFile);

        /*
         * 2. Parsing récursif des instances.
         */
        for (const auto& instanceDescription : description.instances)
        {
            Instance instance = parseInstance(location, instanceDescription);

            assembly->addInstance(std::move(instance));
        }

        /*
         * 3. Liens explicites.
         */
        ExplicitConnectionRegistry explicitConnections;

        for (const auto& link : description.explicitLinks)
        {
            const LinkDescription normalized = normalizeShadowReference(link, ownerInstanceName);

            explicitConnections.add(normalized);

            processLink(*assembly, normalized);
        }

        /*
         * 4. Liens implicites.
         */
        for (const auto& rule : description.implicitLinks)
        {
            processImplicitLinks(*assembly, rule, explicitConnections);
        }

        return assembly;
    }

    /*
     * ========================================================================
     * Parsing d'une instance d'assembly
     * ========================================================================
     */

    Instance parseInstance(const ComponentLocation& parentLocation, const InstanceDescription& description)
    {
        const ComponentLocation* location = nullptr;

        if (description.isComposite())
        {
            /*
             * Un composite peut être situé
             * dans un autre mini-projet.
             */
            location = &_catalog.findGlobal(description.componentType);
        }
        else
        {
            /*
             * Un composant terminal est recherché
             * dans le mini-projet courant.
             */
            location = &_catalog.findLocal(parentLocation.projectDirectory, description.componentType);
        }

        Component component = parseComponent(*location, description.name);

        Instance instance;

        instance.name           = description.name;
        instance.componentType  = description.componentType;
        instance.implementation = description.implementation;
        instance.operations     = component.operations;

        if (!component.isComposite())
        {
            /*
             * Cas terminal :
             *
             * chaque port retourne exactement
             * un endpoint réel.
             */

            for (const auto& operation : component.operations)
            {
                instance.bindings[operation.name].push_back({ instance.name, operation.name });
            }

            registerGlobalInstance(description);
        }
        else
        {
            /*
             * Composite :
             *
             * ses bindings sont simplement ceux
             * accumulés dans son shadow.
             *
             * Ils sont déjà complètement résolus
             * vers des composants terminaux.
             */

            instance.bindings = component.assembly->shadow().bindings;
        }

        return instance;
    }

    /*
     * ========================================================================
     * Création du shadow
     * ========================================================================
     */

    static Instance createShadow(const std::vector<Operation>& operations)
    {
        Instance shadow;

        shadow.name           = Assembly::ShadowInstanceName;
        shadow.componentType  = "$shadow";
        shadow.implementation = "$shadow";
        shadow.shadow         = true;

        /*
         * Copie des ports avec direction inversée.
         */
        for (const auto& operation : operations)
        {
            Operation shadowOperation;
            shadowOperation.name      = operation.name;
            shadowOperation.direction = invertDirection(operation.direction);

            shadow.operations.push_back(std::move(shadowOperation));
        }

        return shadow;
    }

    static OperationDirection invertDirection(OperationDirection direction)
    {
        if (direction == OperationDirection::Sent)
        {
            return OperationDirection::Received;
        }

        return OperationDirection::Sent;
    }

    /*
     * ========================================================================
     * Résolution d'un lien
     * ========================================================================
     */

    void processLink(Assembly& assembly, const LinkDescription& link)
    {
        Instance& sender   = assembly.instance(link.sender.instance);
        Instance& receiver = assembly.instance(link.receiver.instance);

        /*
         * ------------------------------------------------------------
         * shadow -> instance
         *
         * Le port du composite délègue vers une
         * ou plusieurs instances internes.
         * ------------------------------------------------------------
         */
        if (sender.shadow)
        {
            if (receiver.shadow)
            {
                throw std::logic_error("Connexion shadow -> shadow invalide");
            }

            const EndpointList endpoints = receiver.endpoints(link.receiver.operation);

            addBinding(sender, link.sender.operation, endpoints);

            return;
        }

        /*
         * ------------------------------------------------------------
         * instance -> shadow
         *
         * Une ou plusieurs instances internes
         * alimentent le port du composite.
         * ------------------------------------------------------------
         */
        if (receiver.shadow)
        {
            const EndpointList endpoints = sender.endpoints(link.sender.operation);

            addBinding(receiver, link.receiver.operation, endpoints);

            return;
        }

        /*
         * ------------------------------------------------------------
         * instance -> instance
         *
         * Aucun besoin de savoir si sender/receiver
         * sont des composants ou composites.
         *
         * endpoints() renvoie :
         *
         *   composant : un endpoint
         *   composite : N endpoints
         * ------------------------------------------------------------
         */

        const EndpointList senders   = sender.endpoints(link.sender.operation);
        const EndpointList receivers = receiver.endpoints(link.receiver.operation);

        for (const auto& source : senders)
        {
            for (const auto& destination : receivers)
            {
                registerGlobalConnection(source, destination);
            }
        }
    }

    /*
     * ========================================================================
     * Binding du shadow
     * ========================================================================
     */

    static void addBinding(Instance& shadow, const std::string& operation, const EndpointList& endpoints)
    {
        EndpointList& bindings = shadow.bindings[operation];

        for (const Endpoint& endpoint : endpoints)
        {
            const auto it = std::find(bindings.begin(), bindings.end(), endpoint);

            if (it == bindings.end())
            {
                bindings.push_back(endpoint);
            }
        }
    }

    /*
     * ========================================================================
     * Liens implicites
     * ========================================================================
     */

    void processImplicitLinks(Assembly& assembly, const ImplicitLinkRule& rule, const ExplicitConnectionRegistry& explicitConnections)
    {
        /*
         * On parcourt tous les eventSent visibles
         * dans l'assembly, shadow compris.
         */
        for (const auto& senderEntry : assembly.instances())
        {
            const Instance& sender = senderEntry.second;

            /*
             * Si instance != "*", seuls les ports
             * de cette instance participent.
             *
             * Cette fonction est volontairement
             * isolée car elle est facile à adapter
             * si la sémantique exacte diffère.
             */
            if (!instanceParticipates(sender, rule))
            {
                continue;
            }

            for (const auto& senderOperation : sender.operations)
            {
                if (senderOperation.direction != OperationDirection::Sent)
                {
                    continue;
                }

                if (!matchesPrefix(senderOperation.name, rule.prefix))
                {
                    continue;
                }

                if (explicitConnections.senderUsed(sender.name, senderOperation.name))
                {
                    continue;
                }

                findImplicitReceivers(assembly, sender, senderOperation.name, rule, explicitConnections);
            }
        }
    }

    void findImplicitReceivers(Assembly& assembly, const Instance& sender, const std::string& operation, const ImplicitLinkRule& rule, const ExplicitConnectionRegistry& explicitConnections)
    {
        for (const auto& receiverEntry : assembly.instances())
        {
            const Instance& receiver = receiverEntry.second;

            /*
             * Pas de connexion d'une instance
             * vers elle-même.
             *
             * Supprimer ce test si le framework
             * autorise explicitement ce cas.
             */
            if (receiver.name == sender.name)
            {
                continue;
            }

            if (!receiver.hasOperation(operation, OperationDirection::Received))
            {
                continue;
            }

            if (!receiverParticipates(receiver, rule))
            {
                continue;
            }

            if (explicitConnections.receiverUsed(receiver.name, operation))
            {
                continue;
            }

            LinkDescription generatedLink;

            generatedLink.sender = { sender.name, operation };
            generatedLink.receiver = { receiver.name, operation };

            processLink(assembly, generatedLink);
        }
    }

    /*
     * ========================================================================
     * Sélection instance="..."
     * ========================================================================
     *
     * Ici :
     *
     *     instance="*"
     *
     * signifie toutes les instances.
     *
     * Pour :
     *
     *     instance="CompA"
     *
     * on limite le rule à CompA.
     *
     * Si la sémantique réelle du framework est différente,
     * ces deux petites fonctions sont les seules à modifier.
     */

    static bool instanceParticipates(const Instance& instance, const ImplicitLinkRule& rule)
    {
        if (rule.instance.empty() || rule.instance == "*")
        {
            return true;
        }

        return instance.name == rule.instance;
    }

    static bool receiverParticipates(const Instance&, const ImplicitLinkRule& rule)
    {
        /*
         * Une instance explicitement sélectionnée
         * désigne ici la source.
         *
         * Les receivers restent recherchés dans
         * tout l'assembly.
         */
        return !rule.instance.empty();
    }

    /*
     * ========================================================================
     * Prefix implicite
     * ========================================================================
     */

    static bool matchesPrefix(const std::string& operation, const std::string& prefix)
    {
        if (prefix.empty())
        {
            return true;
        }

        if (operation.size() < prefix.size())
        {
            return false;
        }

        return operation.compare(0, prefix.size(), prefix) == 0;
    }

    /*
     * ========================================================================
     * Référence au shadow
     * ========================================================================
     *
     * Si le XML utilise le nom de l'instance du
     * composite parent pour désigner son shadow,
     * cette normalisation permet de le convertir.
     *
     * Exemple :
     *
     *     sender instanceName="CompositeA"
     *
     * devient :
     *
     *     sender instanceName="$shadow"
     */

    static LinkDescription normalizeShadowReference(LinkDescription link, const std::string& ownerInstanceName)
    {
        if (link.sender.instance == ownerInstanceName)
        {
            link.sender.instance = Assembly::ShadowInstanceName;
        }

        if (link.receiver.instance == ownerInstanceName)
        {
            link.receiver.instance = Assembly::ShadowInstanceName;
        }

        return link;
    }

    /*
     * ========================================================================
     * Instance réelle globale
     * ========================================================================
     */

    void registerGlobalInstance(const InstanceDescription& description)
    {
        const auto result = _instanceNames.insert(description.name);

        if (!result.second)
        {
            throw std::logic_error("Instance reelle dupliquee : '" + description.name + "'");
        }

        _architecture.instances.push_back({ description.name, description.componentType, description.implementation });
    }

    /*
     * ========================================================================
     * Connexion réelle globale
     * ========================================================================
     */

    struct ConnectionKey
    {
        std::string senderInstance;
        std::string senderOperation;
        std::string receiverInstance;
        std::string receiverOperation;


        bool operator<(const ConnectionKey& other) const noexcept
        {
            if (senderInstance != other.senderInstance)
            {
                return senderInstance < other.senderInstance;
            }

            if (senderOperation != other.senderOperation)
            {
                return senderOperation < other.senderOperation;
            }

            if (receiverInstance != other.receiverInstance)
            {
                return receiverInstance < other.receiverInstance;
            }

            return receiverOperation < other.receiverOperation;
        }
    };

    void registerGlobalConnection(const Endpoint& sender, const Endpoint& receiver)
    {
        ConnectionKey key{ sender.instance, sender.operation, receiver.instance, receiver.operation };

        /*
         * Evite les doublons produits par plusieurs
         * chemins de délégation/implicitLinks.
         */
        if (!_connectionKeys.insert(key).second)
        {
            return;
        }

        _architecture.connections.push_back({ sender.instance, sender.operation, receiver.instance, receiver.operation });
    }

private:

    std::filesystem::path _componentProjectsDirectory;

    std::filesystem::path _moduleProjectsDirectory;

    ProjectCatalog _catalog;

    ComponentFileParser _componentParser;
    AssemblyFileParser _assemblyParser;

    LoadedArchitecture _architecture;

    std::set<std::string> _instanceNames;

    std::set<ConnectionKey> _connectionKeys;
};

} // namespace architecture
