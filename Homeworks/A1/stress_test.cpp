#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define NUM_CLIENTS 100
#define TEST_DURATION 60 // seconds

struct TestClient {
    int socket;
    std::string username;
    std::string password;
};

// Test users from users.txt
std::vector<std::pair<std::string, std::string>> test_users = {
    {"alice", "password123"},
    {"bob", "qwerty456"},
    {"charlie", "secure789"},
    {"david", "helloWorld!"},
    {"eve", "trustno1"},
    {"frank", "letmein"},
    {"grace", "passw0rd"}
};

std::vector<std::string> test_messages = {
    "/broadcast Hello everyone!",
    "/create_group TestGroup",
    "/join_group TestGroup",
    "/group_msg TestGroup Test message",
    "/leave_group TestGroup",
    "/msg alice Hello there!"
};

void simulate_client(int client_id) {
    TestClient client;
    
    // Create socket
    client.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client.socket < 0) {
        std::cerr << "Client " << client_id << ": Socket creation failed" << std::endl;
        return;
    }

    // Connect to server
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client.socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Client " << client_id << ": Connection failed" << std::endl;
        close(client.socket);
        return;
    }

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> user_dist(0, test_users.size() - 1);
    std::uniform_int_distribution<> msg_dist(0, test_messages.size() - 1);
    std::uniform_int_distribution<> delay_dist(100, 2000); // 100-2000ms delay

    char buffer[BUFFER_SIZE];
    
    // Authentication
    memset(buffer, 0, BUFFER_SIZE);
    recv(client.socket, buffer, BUFFER_SIZE, 0); // "Enter username: "
    
    int user_idx = user_dist(gen);
    client.username = test_users[user_idx].first;
    client.password = test_users[user_idx].second;
    
    send(client.socket, client.username.c_str(), client.username.length(), 0);
    
    memset(buffer, 0, BUFFER_SIZE);
    recv(client.socket, buffer, BUFFER_SIZE, 0); // "Enter password: "
    
    send(client.socket, client.password.c_str(), client.password.length(), 0);
    
    memset(buffer, 0, BUFFER_SIZE);
    recv(client.socket, buffer, BUFFER_SIZE, 0); // Welcome message
    
    // If authentication failed, close connection
    if (std::string(buffer).find("Authentication failed") != std::string::npos) {
        std::cout << "Client " << client_id << ": Authentication failed" << std::endl;
        close(client.socket);
        return;
    }

    std::cout << "Client " << client_id << " (" << client.username << "): Connected and authenticated" << std::endl;

    // Start time for test duration
    auto start_time = std::chrono::steady_clock::now();
    
    // Main test loop
    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= TEST_DURATION) {
            break;
        }

        // Send random message
        std::string message = test_messages[msg_dist(gen)];
        send(client.socket, message.c_str(), message.length(), 0);
        
        // Random delay between messages
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
        
        // Receive any pending messages
        memset(buffer, 0, BUFFER_SIZE);
        recv(client.socket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
    }

    close(client.socket);
    std::cout << "Client " << client_id << " (" << client.username << "): Test completed" << std::endl;
}

int main() {
    std::cout << "Starting stress test with " << NUM_CLIENTS << " clients for " 
              << TEST_DURATION << " seconds..." << std::endl;

    std::vector<std::thread> client_threads;
    
    // Launch client threads
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        client_threads.emplace_back(simulate_client, i);
        // Small delay between client launches to prevent connection flood
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Wait for all clients to complete
    for (auto& thread : client_threads) {
        thread.join();
    }

    std::cout << "Stress test completed." << std::endl;
    return 0;
}
