/**
 * A non-functional sketch code to research
 * what should the program look like.
 * Also decide what the APIs should look like.
 */

#include <memory>
#include <iostream>

namespace opendot {
    
    namespace cxxrpc {

        class pipeline {  };
        void init( int argc, char const *argv[] ) {}
        void finalize() {}
        auto create_pipeline() -> std::unique_ptr<opendot::cxxrpc::pipeline> {}
    
    } // namespace cxxrpc

} // namespace opendot


int main(int argc, char const *argv[]) {

    opendot::cxxrpc::init(argc, argv);
    
    std::unique_ptr<opendot::cxxrpc::pipeline> pipeline
        = opendot::cxxrpc::create_pipeline();
    auto latch = pipeline->start();

    /**
     * finalize will take n number of latches
     * when all latches are free only then finalize and exit
     */
    opendot::cxxrpc::finalize(latch);
    
    return 0;
}
