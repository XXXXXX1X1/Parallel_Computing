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
        // Тип задачи
        using Task = std::function<T()>;

    private:
        // Элемент очереди задач:
        struct TaskItem{
            size_t id;
            Task task;
        };

        // Очередь задач, ожидающих выполнения
        std::queue<TaskItem> tasks;
        // Контейнер для готовых результатов:
        std::unordered_map<size_t, T> results;
        // Мьютекс для синхронизации доступа к общим данным
        std::mutex mtx;
        // Условная переменная для ожидания появления задач
        std::condition_variable cv_tasks;
        // Условная переменная для ожидания готовых результатов
        std::condition_variable cv_results;
        // Рабочий поток сервера
        std::thread worker;

        bool running = false;
        bool stop_flag = false;
        size_t next_id = 1;
        
        void worker_loop() {
            while (true)
            {
                TaskItem item;

                {
                    std::unique_lock<std::mutex> lock(mtx); // захватываем mutex mtx
                
                    // если задача нет, то засыпаем
                    cv_tasks.wait(lock, [this](){
                        return stop_flag || !tasks.empty();
                    });

                    // Если сервер остановлен и задач больше нет,
                    // завершаем поток
                    if (stop_flag && tasks.empty()) {
                        break;
                    }

                    // Забираем задачу из очереди
                    item = std::move(tasks.front());
                    tasks.pop();
                }

                // Выполняем задачу 
                T value = item.task();

                {
                    std::lock_guard<std::mutex> lock(mtx);

                    // Сохраняем результат по id задачи
                    results[item.id] = value;
                }
                
                // Уведомляем ожидающие потоки,
                // что появился новый результат
                cv_results.notify_all();
            }
        }

    public:
        TaskServer() = default;

        // В деструкторе гарантированно останавливаем сервер
        ~TaskServer(){
            stop();
        }

        // Запуск сервера и рабочего потока
        void start() {
            std::lock_guard<std::mutex> lock(mtx);

            if (running){
                return;
            }

            stop_flag = false;
            running = true;

            // Создаём рабочий поток
            worker = std::thread(&TaskServer::worker_loop, this);
        }

        // Остановка сервера
        void stop() {
            {
                std::lock_guard<std::mutex> lock(mtx);

                if (!running){
                    return;
                }

                stop_flag = true;
            }
        
            // Будим рабочий поток, если он ждёт задачи
            cv_tasks.notify_all();

            // Дожидаемся завершения рабочего потока
            if (worker.joinable()){
                worker.join();
            }

            {
                std::lock_guard<std::mutex> lock(mtx);
                running = false;
            }
        }

        // Добавление новой задачи в очередь
        size_t add_task(Task task){
            std::lock_guard<std::mutex> lock(mtx);

            if (!running){
                throw std::runtime_error("Server in not running");
            }

            // Назначаем задаче уникальный id
            size_t id = next_id;
            next_id++;

            // Помещаем задачу в очередь
            tasks.push({id, std::move(task)});

            // Уведомляем рабочий поток о новой задаче
            cv_tasks.notify_one();

            return id;
        }

        // Запрос результата по id задачи
        T request_result(size_t id_res) {
            std::unique_lock<std::mutex> lock(mtx);

            // Ждём, пока результат с данным id не появится
            cv_results.wait(lock, [this, id_res]() {
                return results.find(id_res) != results.end();
            });

            // Копируем результат
            T value = results[id_res];

            // Удаляем его из контейнера,
            results.erase(id_res);

            return value;
        }
};