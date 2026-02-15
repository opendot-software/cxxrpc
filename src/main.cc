/**
 * A non-functional sketch code to research
 * what should the program look like.
 * Also decide what the APIs should look like.
 */

#include <memory>
#include <iostream>
#include <chrono>
#include <functional>

using namespace std::chrono_literals;

namespace opendot {
    
    namespace cxxrpc {

        struct pipeline_config_s {
            int num_stages;
            int stage_distribution_policy;
            std::chrono::milliseconds tick_resolution;
        };

        class pipeline { 
            public:
                bool start() {}
                bool configure(pipeline_config_s&& _pconfig) {

				}
				void add_stage(pipeline_stage& _pstage) {}
				bool bake() {  }
				bool verify() {
					// ---- this method will attempt to deduce all possible errors at compile time ---- //

					/**
					 * verify if
					 *   - output of one stage is compatible with input of the next stage
					 *   - num of stages added and specified in config is a mismatch
					 */

					 // ------------------------------------------------------------------------------- //
				}
        };

		class pipeline_stage {
				std::function<void(void)> m_stagebody;
			public:
				pipeline_stage(std::function<void(void)>&& _stagebody) : m_stagebody(_stagebody) {}
				bool operator()() { m_stagebody(); }
		};

        void init( int argc, char const *argv[] ) {}
        void finalize(std::initializer_list<bool>&& _) {}
        auto create_pipeline() -> std::unique_ptr<opendot::cxxrpc::pipeline> {}
    
    } // namespace cxxrpc

} // namespace opendot


int main(int argc, char const *argv[]) {

    opendot::cxxrpc::init(argc, argv);
    
    std::unique_ptr<opendot::cxxrpc::pipeline> pipeline
        = opendot::cxxrpc::create_pipeline();
    
    bool status = pipeline->configure({
        .num_stages                 = 4,
        .stage_distribution_policy  = 1,
        .tick_resolution            = 10ms,
    });
	opendot::cxxrpc::pipeline_stage encryption_stage([](){});
	opendot::cxxrpc::pipeline_stage computing_stage([](){});
	opendot::cxxrpc::pipeline_stage decryption_stage([](){});
	
    pipeline->add_stage(decryption_stage);
    pipeline->add_stage(computing_stage);
    pipeline->add_stage(encryption_stage);
	
	pipeline->bake();
	// -- OR -- //
	pipeline->verify();

    auto latch = pipeline->start();

    /**
     * finalize will take n number of latches
     * when all latches are free only then finalize and exit
     */
    opendot::cxxrpc::finalize({latch});
    
    return 0;
}
