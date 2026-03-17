#pragma once

#include <queue>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>


template<typename T>
class TaskServer {

    public:
        using Task = std::function<T()>;

    private:
        struct TaskItem{
            size_t id;
            Task task;
        };

        std::queue<TaskItem> tasks; // очередь задач
        std::unordered_map<size_t, T> results; // контейнер задач

        std::mutex mtx;
        std::condition_variable cv_tasks;
        std::condition_variable cv_results;

        std::thread worker;

        bool running = false;
        bool stop_flag = false;
        size_t next_id = 1;
        
        void worker_loop() {
            while (true)
            {
                TaskItem item;
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    
                    cv_tasks.wait(lock, [this](){
                        return stop_flag || !tasks.empty();
                    });

                    if (stop_flag && tasks.empty()) {
                        break;
                    }

                    item = std::move(tasks.front());
                    tasks.pop();
                }

                T value = item.task();

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    results[item.id] = value;
                }
                
                cv_results.notify_all();
            }
            
        }
    public:
        TaskServer() = default;
        ~TaskServer(){
            stop();
        }

        void start() {
            std::lock_guard<std::mutex> lock(mtx);

            if (running){
                return;
            }

            stop_flag = false;
            running = true;
            worker = std::thread(&TaskServer::worker_loop, this);

        }

        void stop() {
            {
            std::lock_guard<std::mutex> lock(mtx);

            if (!running){
                return;
            }

            stop_flag = true;
        }
        
        cv_tasks.notify_all();

        if (worker.joinable()){
            worker.join();
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            running = false;
        }

    }

    size_t add_task(Task task){
        std::lock_guard<std::mutex> lock(mtx);

        if (!running){
            throw std::runtime_error("Server in not running");
        }

        size_t id = next_id;
        next_id++;

        tasks.push({id, std::move(task)});
        cv_tasks.notify_one();

        return id;
    }

    T request_result(size_t id_res) {
        std::unique_lock<std::mutex> lock(mtx);

        cv_results.wait(lock, [this, id_res]() {
            return results.find(id_res) != results.end();
        });

        T value = results[id_res];
        results.erase(id_res);

        return value;
    }
};  