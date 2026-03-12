#include <zmq.hpp>
#include <string>
#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <json/json.h>
#include <sstream>

#pragma comment(lib, "user32.lib")

// Optimized function to move the cursor
inline void move_cursor(float x, float y) {
    SetCursorPos(static_cast<int>(x), static_cast<int>(y));
}

// Optimized function for mouse clicks
inline void mouse_click(bool left, bool down) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = left ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP) :
        (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
    SendInput(1, &input, sizeof(INPUT));
}

// Optimized function for double click
inline void double_click() {
    mouse_click(true, true);
    mouse_click(true, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Small delay for double-click
    mouse_click(true, true);
    mouse_click(true, false);
}

// Optimized function for recent apps toggle (Win + Tab)
inline void toggle_recent_apps() {
    INPUT inputs[4] = { 0 };
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_TAB;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

// Optimized function for drag start and stop
inline void drag_start() {
    mouse_click(true, true);
}

inline void drag_stop() {
    mouse_click(true, false);
}

// Function to execute browser open command
void execute_browser_command(const std::string& url) {
    if (url.empty() || (url.find("http://") != 0 && url.find("https://") != 0)) {
        std::cerr << "Invalid URL format: " << url << std::endl;
        return;
    }

    std::string fullCommand = "start \"\" \"" + url + "\"";
    int result = system(fullCommand.c_str());
    if (result != 0) {
        std::cerr << "Browser command failed with code: " << result << std::endl;
    }
}

// Check if input is JSON
bool try_parse_json(const std::string& input, Json::Value& outRoot) {
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(input);
    return Json::parseFromStream(reader, stream, &outRoot, &errors);
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_ROUTER);
    socket.bind("tcp://*:5555");

    std::cout << "GESCON Backend running...\n";

    while (true) {
        zmq::message_t client_id;
        zmq::message_t request;

        if (!socket.recv(client_id, zmq::recv_flags::none) || !socket.recv(request, zmq::recv_flags::none)) {
            continue;
        }

        std::string command(static_cast<char*>(request.data()), request.size());
        std::string response = "OK";

        Json::Value root;
        bool is_json = try_parse_json(command, root);

        if (is_json) {
            if (!root.isMember("command") || !root["command"].isString()) {
                std::cerr << "Invalid JSON: 'command' missing or not a string." << std::endl;
                response = "Error: Invalid JSON format";
            }
            else {
                std::string cmd = root["command"].asString();

                if (cmd == "open_url" || cmd == "browser_open") {
                    if (root.isMember("parameters") && root["parameters"].isMember("url")) {
                        std::string url = root["parameters"]["url"].asString();
                        execute_browser_command(url);
                    }
                    else {
                        std::cerr << "Missing URL in parameters." << std::endl;
                        response = "Error: URL missing";
                    }
                }
                else if (cmd == "google_search") {
                    if (root.isMember("parameters") && root["parameters"].isMember("query")) {
                        std::string query = root["parameters"]["query"].asString();
                        std::string search_url = "https://www.google.com/search?q=" + query;
                        execute_browser_command(search_url);
                    }
                    else {
                        std::cerr << "Missing query in parameters." << std::endl;
                        response = "Error: Query missing";
                    }
                }
                else {
                    std::cerr << "Unsupported JSON command: " << cmd << std::endl;
                    response = "Error: Unsupported JSON command";
                }
            }
        }
        else {
            // Handle plain text commands
            if (command.find("move") == 0) {
                float x, y;
                sscanf_s(command.c_str(), "move %f %f", &x, &y);
                move_cursor(x, y);
            }
            else if (command == "click") {
                mouse_click(true, true);
                mouse_click(true, false);
            }
            else if (command == "double_click") {
                double_click();
            }
            else if (command == "right_click") {
                mouse_click(false, true);
                mouse_click(false, false);
            }
            else if (command == "mouse_down") {
                mouse_click(true, true);
            }
            else if (command == "mouse_up") {
                mouse_click(true, false);
            }
            else if (command == "recent_apps_toggle") {
                toggle_recent_apps();
            }
            else if (command == "drag_start") {
                drag_start();
            }
            else if (command == "drag_stop") {
                drag_stop();
            }
            else if (command.find("browser_open") == 0) {
                size_t space_pos = command.find(' ');
                if (space_pos != std::string::npos) {
                    std::string url = command.substr(space_pos + 1);
                    execute_browser_command(url);
                }
                else {
                    std::cerr << "Invalid browser_open format." << std::endl;
                    response = "Error: Invalid browser_open format";
                }
            }
            else if (command.find("google_search") == 0) {
                size_t space_pos = command.find(' ');
                if (space_pos != std::string::npos) {
                    std::string query = command.substr(space_pos + 1);
                    std::string search_url = "https://www.google.com/search?q=" + query;
                    execute_browser_command(search_url);
                }
                else {
                    std::cerr << "Invalid google_search format." << std::endl;
                    response = "Error: Invalid google_search format";
                }
            }
            else {
                std::cerr << "Unknown command: " << command << std::endl;
                response = "Error: Unknown command";
            }
        }

        // Send the response back
        zmq::message_t reply(response.size());
        memcpy(reply.data(), response.c_str(), response.size());
        socket.send(client_id, zmq::send_flags::sndmore);
        socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}
