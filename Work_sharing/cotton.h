#include <functional>
namespace cotton{
    void start_finish();
    void end_finish();
    void init_runtime();
    void finalize_runtime();
    void async(std::function<void()> &&lambda);
}