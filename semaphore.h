/*
 * Implement a simple semaphore interface:
 *
 *    wait()
 *    post()
 *    trywait()
 *
 * There are two implementations available:
 *
 * - "semaphore" is blocking version that is implemented based on mutex and
 *   condition variable.
 *
 * - "semaphore_lockfree" is non-blocking version that is implemented based on
 *   c++ atomic CAS (compare and swap).
 *
 */

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace std {

   class semaphore_interface {
   public:
      virtual void wait(void) = 0;
      virtual void post(void) = 0;
      virtual bool trywait(void) = 0;
   };

   class semaphore : public semaphore_interface {
      mutex m_mtx;
      condition_variable m_cv;
      volatile int m_val;
   public:
      semaphore(int m_val) : m_val(m_val) {}
      virtual void wait(void) {
         unique_lock<mutex> lck(m_mtx);
         m_cv.wait(lck, [&](void)->bool {return m_val > 0;});
         m_val--;
         // lck will be unlocked after out of scope.
      }
      virtual void post(void) {
         unique_lock<mutex> lck(m_mtx);
         m_val++;
         if (m_val > 0) m_cv.notify_all();
         // lck will be unlocked after out of scope.
      }
      virtual bool trywait(void) {
         unique_lock<mutex> lck(m_mtx);
         if (m_val > 0) {
            m_val--;
            return true;
         } else {
            return false;
         }
      }
   };

   class semaphore_lockfree : public semaphore_interface {
      atomic<int> m_val;
   public:
      semaphore_lockfree(int val) {m_val = val;}
      virtual void wait(void) {
         while (true) {
            this_thread::yield();

            int old_val = m_val;

            if (old_val <= 0) {
               continue;
            }

            if (m_val.compare_exchange_strong(old_val, old_val - 1)) {
               break;
            }
         }
      }
      virtual void post(void) {
         m_val++;
      }
      virtual bool trywait(void) {
         int old_val = m_val;

         if (old_val <= 0) {
            return false;
         }

         if (m_val.compare_exchange_strong(old_val, old_val - 1)) {
            return true;
         }

         return false;
      }
   };
}
