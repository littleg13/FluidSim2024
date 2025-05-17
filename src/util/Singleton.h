#include <memory>

template <typename T>
class Singleton
{
public:
    template <typename... Us>
    static T* CreateInstance(Us... Args)
    {
        Instance = std::make_unique<T>(std::forward<Us>(Args)...);
        return Instance.get();
    };

    // We purposefully don't do a validation check here for the sake of performance
    // Who would write bad code anyways, right...?
    static T* GetInstance()
    {
        return Instance.get();
    };

    static void DestroyInstance()
    {
        Instance.reset();
    };

private:
    static inline std::unique_ptr<T> Instance = nullptr;
};