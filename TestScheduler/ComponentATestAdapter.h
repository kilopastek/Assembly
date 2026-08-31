class ComponentATestAdapter
    : public test::IScheduledComponent
{
public:
    ComponentATestAdapter(ComponentA& component, TestMessaging& messaging)
        : component_{component},
          messaging_{messaging}
    {
    }

    const std::string&
    name() const noexcept override
    {
        static const std::string name{"ComponentA"};
        return name;
    }

    bool hasPendingWork() const override
    {
        return messaging_.hasPendingMessage(
            "ComponentA");
    }

    void executeOneCycle() override
    {
        component_.executeOneCycle();
    }

private:
    ComponentA& component_;
    TestMessaging& messaging_;
};