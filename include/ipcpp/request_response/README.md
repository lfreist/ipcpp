# Request/Response
Shm allocated data provided on request

## Lazy
New data are created on requests

## Continually
- New data are continually created and next data are provided on requests.
- Provider tracks requests and releases reference count managed objects if they are acquired by consumer
- next data are never deallocated
- New data are allocated by swapping next and second next blocks. second next block can be deallocated if they were not requested