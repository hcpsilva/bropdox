#pragma once

#include "helpers/FileHandler.hpp"

#include "util/Exception.hpp"
#include "util/FileData.hpp"

#include "inotify-cpp/NotifierBuilder.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

#define DAEMON_SLEEP_SECONDS 5

using namespace inotify;

class Watcher {
    FileHandler file_handler;
    NotifierBuilder notifier;

    bool running = false;
    std::vector<bdu::file_event_t> modified_files;

public:
    /**
     * Runs the main watching loop.
     * 
     * @throws exception_here
     */
    void run();

    /**
     * Stops the service.
     */
    void stop();

    /**
     * Checks if the watcher is running.
     * 
     * @return the is_running bool
     */
    bool is_running();

    /**
     * Gets and cleans the modifications vector.
     * 
     * @return the modifications vector
     */
    std::vector<bdu::file_event_t> get_events();

private:
    /**
     * Gets called when a file gets modified.
     */
    void handle_file_modification(Notification event);

public:
    Watcher() = default;

    Watcher(std::string user_id);
};