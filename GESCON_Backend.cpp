#include <zmq.hpp>
#include <string>
#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>

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
    mouse_click(true, true);  // Mouse down (left click)
}

inline void drag_stop() {
    mouse_click(true, false); // Mouse up (left click)
}

int main() {
    // Initialize ZeroMQ context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    std::cout << "GESCON Backend running...\n";

    while (true) {
        // Receive message
        zmq::message_t request;
        if (!socket.recv(request, zmq::recv_flags::none)) {
            continue;
        }

        std::string command(static_cast<char*>(request.data()), request.size());
        std::string response = "OK";

        // Parse and execute commands
        std::cout << "Received command: " << command << std::endl;

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
        else if (command == "exit") {
            break;
        }

        // Send acknowledgment
        zmq::message_t reply(response.size());
        memcpy(reply.data(), response.c_str(), response.size());
        socket.send(reply, zmq::send_flags::none);
    }

    std::cout << "GESCON Backend shutting down...\n";
    return 0;
}
