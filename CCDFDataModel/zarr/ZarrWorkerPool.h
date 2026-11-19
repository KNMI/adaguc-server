/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  A small pool of threads to decode Zarr chunks in parallel
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-11-19
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef ZARRWORKERPOOL_H
#define ZARRWORKERPOOL_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Zarr {

  /**
   * Runs tasks on a fixed number of threads. Submitting blocks while maxQueued tasks are waiting, so that downloaded chunks
   * do not pile up in memory when decoding is slower than downloading. Tasks must not throw.
   */
  class WorkerPool {
  public:
    WorkerPool(size_t numThreads, size_t maxQueued) : maxQueued(maxQueued) {
      for (size_t index = 0; index < numThreads; index++) {
        threads.emplace_back([this] { run(); });
      }
    }

    /** Finishes the tasks that were submitted, then stops the threads */
    ~WorkerPool() {
      {
        std::lock_guard<std::mutex> lock(mutex);
        stopping = true;
      }
      hasWork.notify_all();
      for (auto &thread: threads) thread.join();
    }

    void submit(std::function<void()> task) {
      std::unique_lock<std::mutex> lock(mutex);
      hasRoom.wait(lock, [this] { return queue.size() < maxQueued; });
      queue.push_back(std::move(task));
      hasWork.notify_one();
    }

    /** Waits until all submitted tasks have run */
    void wait() {
      std::unique_lock<std::mutex> lock(mutex);
      allDone.wait(lock, [this] { return queue.empty() && busy == 0; });
    }

  private:
    void run() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(mutex);
          hasWork.wait(lock, [this] { return stopping || !queue.empty(); });
          if (queue.empty()) return;
          task = std::move(queue.front());
          queue.pop_front();
          busy++;
        }
        hasRoom.notify_one();
        task();
        {
          std::lock_guard<std::mutex> lock(mutex);
          busy--;
          if (queue.empty() && busy == 0) allDone.notify_all();
        }
      }
    }

    const size_t maxQueued;
    std::vector<std::thread> threads;
    std::deque<std::function<void()>> queue;
    std::mutex mutex;
    std::condition_variable hasWork, hasRoom, allDone;
    size_t busy = 0;
    bool stopping = false;
  };

} // namespace Zarr

#endif
