#include <functional>
namespace cotton{
    // Initialising the finish_counter
    void start_finish();
    // End block which will also complete the remaining tasks from the task pool and wait for the completion of all tasks
    void end_finish();
    // To initialise all the runtime values
    void init_runtime();
    // To shutdown the program
    void finalize_runtime();
    // Function to create an asynchronous task by passing a lambda function to it
    void async(std::function<void()> &&lambda);

    int get_num_workers();
}