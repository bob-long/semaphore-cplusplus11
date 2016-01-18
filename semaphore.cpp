/*
 * Driver program of the semaphore class.
 *
 * This driver program implements a simple multiple producer/consumer model
 */

#include <iostream>
#include <thread>
#include <random>
#include <chrono>

#include "semaphore.h"

using namespace std;

#define BUF_LEN 16
#define MAX_PRODUCT_COUNT 32
#define NUM_PROD_THREADS 2
#define NUM_CONS_THREADS 4

minstd_rand0 rnd;

int start = 0;
int tail = 0;
int buffer[BUF_LEN] = { 0 };

atomic<int> n_prod(0);
atomic<int> n_cons(0);
atomic<int> prod_count(0);

/*
 * Semaphore to ensure consumer will not read if the buffer is empty
 */
semaphore_interface *empty;

/*
 * Semaphore to ensure producer will not producing if the buffer is full
 */
semaphore_interface *full;

/*
 * Only one consumer can dequeue an item at a time.
 */
semaphore_interface *cs;

/*
 * Only one producer can enqueue an item at a time.
 */
semaphore_interface *pd;

/*
 * main thread wait to quit.
 */
semaphore_interface *quit;

void producer_func(int pid) {
   n_prod++;
   while (true) {
      this_thread::sleep_for(chrono::seconds(1));

      /*
       * Only produce if there is empty slot to produce.
       */
      empty->wait();

      /*
       * Only one producer to access the buffer for enqueuing at a time.
       */
      pd->wait();

      /*
       * Reach the production limit, prepare to quit.
       */
      if (prod_count >= MAX_PRODUCT_COUNT) {
         /*
          * Unblock other producer that's waiting.
          */
         pd->post();

         /*
          * Notify the main thread can start quitting.
          */
         quit->post();

         n_prod--;
         break;
      }

      buffer[tail] = rnd();
      cout << "producer:(" << pid << ")" << buffer[tail] << "->" << tail << endl;
      tail = (tail + 1) % BUF_LEN;

      /*
       * Other producer can proceed to access the buffer
       */
      pd->post();

      /*
       * Consumer can start consuming the buffer.
       */
      full->post();
   }
}

void consumer_func(int cid) {
   n_cons++;
   while (true) {
      int sleep = (rnd() % 3) + 1;
      /*
       * Sleep for random amount of seconds (1 ~ 3)
       */
      this_thread::sleep_for(chrono::seconds(sleep));

      /*
       * Only consume if there is at least one item in the buffer.
       */
      full->wait();

      /*
       * Only one consumer can access the buffer for dequeuing at a time.
       */
      cs->wait();

      if (prod_count >= MAX_PRODUCT_COUNT) {
         /*
          * Reach the production limit, prepare to quit.
          */
         cs->post();

         /*
          * Notify the main thread can start quitting.
          */
         quit->post();
         n_cons--;
         break;
      }

      cout << "consumer:(" << cid << ")" << start << "<-" << buffer[start] << endl;
      start = (start + 1) % BUF_LEN;

      /*
       * Record the product count
       */
      prod_count++;

      /*
       * Other consumer can proceed to access the buffer.
       */
      cs->post();

      /*
       * Notify the producer that at least one slot is ready for producing.
       */
      empty->post();
   }
}

int main(int argc, char *argv[]) {
   thread producers[NUM_PROD_THREADS];
   thread consumers[NUM_CONS_THREADS];

   full = new semaphore_lockfree(0);
   empty = new semaphore_lockfree(BUF_LEN);
   pd = new semaphore_lockfree(1);
   cs = new semaphore_lockfree(1);
   quit = new semaphore(0);

   rnd.seed(chrono::system_clock::now().time_since_epoch().count());

   for (int i = 0; i < NUM_PROD_THREADS; i++) {
      producers[i] = thread(producer_func, i);
   }

   for (int i = 0; i < NUM_CONS_THREADS; i++) {
      consumers[i] = thread(consumer_func, i);
   }

   quit->wait();

   while (n_prod || n_cons) {
      full->post();
      empty->post();
   }

   for (int i = 0; i < NUM_PROD_THREADS; i++) {
      producers[i].join();
   }

   for (int i = 0; i < NUM_CONS_THREADS; i++) {
      consumers[i].join();
   }

   return 0;
}
