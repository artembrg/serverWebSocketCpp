#include <iostream>
#include <uwebsockets/App.h>
#include <thread>
#include <algorithm>
#include <sstream>
#include <atomic>
using namespace std;

// Converting anything possible to string
template <typename T>
string toString(T convertion) {
    ostringstream ostream;
    ostream << convertion;
    return ostream.str();
}

// Is number
bool isNum(string data) {
    bool res = true;
    int i = 0;
    while (res && i < data.length()) {
        res = isdigit(data[i]);
        i++;
    }
    return res;
}

// Struct to save connected user data
struct Connection {
    unsigned long counter;
};

int main()
{
    srand(time(0));

    // Vector of threads for multiprocessing realization
    vector<thread*> threads(thread::hardware_concurrency());

    // Every thread in this vector is a server
    transform(threads.begin(), threads.end(), threads.begin(), [](auto* thr) {
        return new thread([]() {

            // Configure application
            uWS::App().ws<Connection>("/*", {

                // When the new user is connecting
                .open = [](auto* ws) {
                    Connection* data = (Connection*)ws->getUserData();
                    data->counter = 0;
                    ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                    cout << "New user!\n";
                },

                // When the user is sending anything
                /*
                    PROTOCOL
                    1) "INC" => counter++;
                    2) "ADD_<num>" => counter += num;
                    3) "SET_<num>" => counter = num;
                    4) "RND_<A>_<B> => counter = A + rand() % (B - A + 1) if A < B else counter = B + rand() % (A - B + 1);
                    5) "*" => sending "Error";
                */
                .message = [](auto* ws, string_view message, uWS::OpCode opCode) {
                    Connection* data = (Connection*)ws->getUserData();

                    // Extracting instruction name
                    string instruction = string(message.substr(0, 3));

                    // Logic for "INC" instruction
                    if (instruction == "INC") {
                        data->counter++;
                        ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                    }

                    // Logic for "ADD" instruction
                    else if (instruction == "ADD") {
                        string num_str = string(message).substr(4);
                        if (isNum(num_str)) {
                            unsigned long num = stoul(num_str);
                            data->counter += num;
                            ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                        }
                        else ws->send("Error", uWS::OpCode::TEXT, false);
                    }

                    // Logic for "SET" instruction
                    else if (instruction == "SET") {
                        string num_str = string(message).substr(4);
                        if (isNum(num_str)) {
                            unsigned long num = stoul(num_str);
                            data->counter = num;
                            ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                        }
                        else ws->send("Error", uWS::OpCode::TEXT, false);
                    }

                    // Logic for "RND" instruction
                    else if (instruction == "RND") {
                        string rnd_data = string(message).substr(4);
                        unsigned int underscope_ind = 0;
                        unsigned int i = 1;
                        while (underscope_ind == 0 && i < rnd_data.length()) {
                            if (rnd_data[i] == '_') {
                                underscope_ind = i;
                            }
                            i++;
                        }
                        if (underscope_ind != 0) {
                            string A_str = rnd_data.substr(0, underscope_ind);
                            string B_str = rnd_data.substr(underscope_ind + 1);
                            if (isNum(A_str) && isNum(B_str)) {
                                unsigned long A = stoul(A_str);
                                unsigned long B = stoul(B_str);
                                if (A < B) {
                                    data->counter = A + rand() % (B - A + 1);
                                    ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                                }
                                else {
                                    data->counter = B + rand() % (A - B + 1);
                                    ws->send(string_view(toString(data->counter)), uWS::OpCode::TEXT, false);
                                }
                            }
                            else ws->send("Error", uWS::OpCode::TEXT, false);
                        }
                        else ws->send("Error", uWS::OpCode::TEXT, false);
                    }

                    // Response for unknown instruction
                    else {
                        ws->send("Error", uWS::OpCode::TEXT, false);
                    }
                }

            // Listening port number and server start status
            }).listen(9000, [](auto* listenSocket) {
                if (listenSocket) cout << "Server started and listening on port 9000\n";
                else cout << "Failed!\n";

            // Running configured application
            }).run();
        });
    });

    // Server stop
    for_each(threads.begin(), threads.end(), [](auto* thr) {thr->join();});
}

