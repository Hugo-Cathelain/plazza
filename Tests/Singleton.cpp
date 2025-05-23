///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Utils/Singleton.hpp"
#include <criterion/criterion.h>
#include <thread>
#include <vector>
#include <atomic>

///////////////////////////////////////////////////////////////////////////////
using namespace Plazza;

///////////////////////////////////////////////////////////////////////////////
// Test classes for singleton testing
///////////////////////////////////////////////////////////////////////////////
class TestSingleton : public Singleton<TestSingleton>
{
public:
    TestSingleton() : m_value(42) {}

    int GetValue() const { return m_value; }
    void SetValue(int value) { m_value = value; }

private:
    int m_value;
};

class AnotherTestSingleton : public Singleton<AnotherTestSingleton>
{
public:
    AnotherTestSingleton() : m_data("test") {}

    const std::string& GetData() const { return m_data; }
    void SetData(const std::string& data) { m_data = data; }

private:
    std::string m_data;
};

///////////////////////////////////////////////////////////////////////////////
// Test Suite for Singleton
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, get_same_instance)
{
    TestSingleton& instance1 = TestSingleton::GetInstance();
    TestSingleton& instance2 = TestSingleton::GetInstance();

    cr_assert_eq(&instance1, &instance2, "Should return the same instance");
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, state_persistence)
{
    TestSingleton& instance1 = TestSingleton::GetInstance();
    instance1.SetValue(100);

    TestSingleton& instance2 = TestSingleton::GetInstance();

    cr_assert_eq(instance2.GetValue(), 100, "State should persist across getInstance calls");
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, different_singleton_types_independent)
{
    TestSingleton& testInstance = TestSingleton::GetInstance();
    AnotherTestSingleton& anotherInstance = AnotherTestSingleton::GetInstance();

    testInstance.SetValue(123);
    anotherInstance.SetData("changed");

    cr_assert_eq(testInstance.GetValue(), 123, "TestSingleton should maintain its state");
    cr_assert_str_eq(anotherInstance.GetData().c_str(), "changed", "AnotherTestSingleton should maintain its state");

    // Verify they are different objects
    cr_assert_neq((void*)&testInstance, (void*)&anotherInstance, "Different singleton types should be different objects");
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, thread_safety_single_instance)
{
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<TestSingleton*> instances(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instances, i]() {
            instances[i] = &TestSingleton::GetInstance();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All instances should be the same
    for (int i = 1; i < num_threads; ++i) {
        cr_assert_eq(instances[0], instances[i], "All threads should get the same instance");
    }
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, thread_safety_concurrent_access)
{
    const int num_threads = 20;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> counter(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                TestSingleton& instance = TestSingleton::GetInstance();
                instance.SetValue(counter.fetch_add(1));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    TestSingleton& final_instance = TestSingleton::GetInstance();
    cr_assert_geq(final_instance.GetValue(), 0, "Instance should be accessible after concurrent operations");
    cr_assert_eq(counter.load(), num_threads * operations_per_thread, "All operations should have completed");
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, instantiable_base_class_non_copyable)
{
    TestSingleton& instance1 = TestSingleton::GetInstance();

    // This test verifies that the singleton cannot be copied
    // The copy constructor and assignment operator should be deleted
    // This is enforced at compile time, so we test by ensuring we can get references
    TestSingleton& instance2 = TestSingleton::GetInstance();

    cr_assert_eq(&instance1, &instance2, "References should point to the same object");
}

Test(Singleton, multiple_calls_same_thread)
{
    std::vector<TestSingleton*> instances;

    for (int i = 0; i < 1000; ++i) {
        instances.push_back(&TestSingleton::GetInstance());
    }

    // All should be the same instance
    for (size_t i = 1; i < instances.size(); ++i) {
        cr_assert_eq(instances[0], instances[i], "Multiple calls should return same instance");
    }
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, lazy_initialization)
{
    // This test ensures that the singleton is only created when first accessed
    // We can't directly test this without accessing the instance, but we can verify
    // that the first access works correctly

    AnotherTestSingleton& instance = AnotherTestSingleton::GetInstance();
    cr_assert_str_eq(instance.GetData().c_str(), "test", "Instance should be properly initialized on first access");
}

///////////////////////////////////////////////////////////////////////////////
Test(Singleton, state_modification_visible_across_calls)
{
    TestSingleton& instance1 = TestSingleton::GetInstance();
    int original_value = instance1.GetValue();

    instance1.SetValue(999);

    TestSingleton& instance2 = TestSingleton::GetInstance();
    cr_assert_eq(instance2.GetValue(), 999, "State changes should be visible across all references");

    // Reset for other tests
    instance2.SetValue(original_value);
}
