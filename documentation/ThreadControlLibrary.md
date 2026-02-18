# Thread Lifecycle Control & Management

This is supposed to be a swc which can spawn n threads and can handle other processing apart from the actual computation work

## Potential Class Names

- `opendot::cxxrpc::thread_pool<>`
- `opendot::cxxrpc::thread_controller<>`
- `opendot::cxxrpc::thread_manager<>`

## Features

- User can spawn n threads on a cpu with 2n cores. This should be allowed
- This class entity should keep track of which core has how many threads allocated for ease in future to spawn more threads in concurrency
