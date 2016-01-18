# semaphore c++11
Implement a simple semaphore interface:
 
  wait()
  post()
  trywait()
 
  There are two implementations available:
 
  - "semaphore" is blocking version that is implemented based on mutex and
    condition variable.
 
  - "semaphore_lockfree" is lock-less version that is implemented based on
    c++ atomic CAS (compare and swap).
