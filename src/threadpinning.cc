// #undef __cplusplus
// #define __cplusplus 202002

#include <hwloc.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>

namespace opendot
{
    namespace cxxrpc
    {

        enum class execution_type_e {
            lazy, eager
        };
        class thread : public std::thread {
            public:
                using std::thread::thread;
                
        };

        
    } // namespace cxxrpc
    
} // namespace opendot

// ---- permanent handle for each thread in pool ---- //
void thread_handle() {
    /**
     * step 1: execute thread placement and pinning to cores
     * step 2: manage numa memory regions
     * step 3: wait on threadpool latch to start all threads together
     * step 4: wait on private work queue for work
     * step 5: keep working until terminate is signalled
     */
}
// -------------------------------------------------- //

struct thread_mapping {
    int num_threads;
    std::initializer_list<int> mapping;
};

class default_pinning_policy {
    public:
        void operator()() { std::printf("default_pinning_policy...\n"); }
};

struct pool_params_s {
    bool enable_hyperthreading;
    bool numa_awareness;
    int num_threads;
};

template <typename pinning_policy_t = default_pinning_policy>
class thread_pool {
    private:
        pinning_policy_t m_ppolicy;

        hwloc_topology_t m_topology;

        int m_num_cores;
        int m_num_hyperthreads;
        int m_num_cpu_sockets;
        
        std::vector<std::unique_ptr<opendot::cxxrpc::thread>> m_threads;

    private:
        void _hwloc_init() {

            hwloc_topology_init(&m_topology);
            hwloc_topology_load(m_topology);

            // Number of physical cores
            m_num_cores         = hwloc_get_nbobjs_by_type(m_topology, HWLOC_OBJ_CORE);

            // Number of logical CPUs (PUs, includes SMT threads)
            m_num_hyperthreads  = hwloc_get_nbobjs_by_type(m_topology, HWLOC_OBJ_PU);

            // Number of CPU packages (sockets)
            m_num_cpu_sockets   = hwloc_get_nbobjs_by_type(m_topology, HWLOC_OBJ_PACKAGE);

        }

    public:
        thread_pool() {
            _hwloc_init();
            m_ppolicy();
        }
        thread_pool( pool_params_s&& _pparams ) {
            _hwloc_init();
            m_ppolicy();
        }
        thread_pool(thread_mapping&& _tmapping) {
            _hwloc_init();
            m_ppolicy();
            for(int idx=0;idx<m_num_hyperthreads;idx++) {
                // m_threads.push_back()
            }
        }
        ~thread_pool() {
            hwloc_topology_destroy(m_topology);            
        }
};

double worker(int _core_id)
{
    // Initialize topology
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    // Get Core 0
    hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, _core_id);

    if (!core) {
        std::cerr << "Core " << _core_id << " not found\n";
        return {};
    }

    // Get first PU inside Core 0
    hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type(
        topology,
        core->cpuset,
        HWLOC_OBJ_PU,
        0
    );

    if (!pu) {
        std::cerr << "PU inside Core 0 not found\n";
        return {};
    }

    // Bind this thread to that PU
    if (hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD) != 0) {
        std::cerr << "Failed to bind thread\n";
    } else {
        std::cout << "Thread pinned to Core" << _core_id << "(PU "
                  << pu->logical_index << ")\n";
    }

    // Simulate work
    double a = 99.99;
    for(int i=0;i<100000000;i++) {
        a = a / 27.786;
        a = a * 27.786;
    }

    hwloc_topology_destroy(topology);
    return a;
}

class pinning_policy {
    public:
        void operator()() { std::printf("custom pinning_policy...\n"); }
};

class thread_controller {};
class thread_manager {};

int main()
{

    // thread_pool tp({.num_threads = 30, .mapping = {16, 2, 2, 2, 2, 2, 2, 2}});
    thread_pool<pinning_policy> tp({
        .enable_hyperthreading = true,
        .numa_awareness = true,
        .num_threads = 32
    });

    opendot::cxxrpc::thread t_array[8];
    for(int i=0;i<8;i++) {
        t_array[i]
            = opendot::cxxrpc::thread(worker, i);
    }
    for(int i=0;i<8;i++) {
        t_array[i].join();
    }
    
    return 0;
}