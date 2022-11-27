
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>
#include "nlohmann/json.hpp"

using std::cin;
using std::cout;
using std::endl;
using json = nlohmann::json;



// ====================== DATA ======================

struct PropertyValue {
	std::string value;
	int last_updated_timestamp;  // The timestamp when this value was last updated
};
struct User {
	std::map<std::string, PropertyValue> properties;
	std::mutex mtx;  // Prevent 2 threads from modifying this user simultaneously
};

std::map<std::string, User> users;

std::vector<json> input_file;  // List of all lines in input file



// ====================== UPDATE FUNCTIONS ======================

void update(const std::string &username, int timestamp, const json &values) {
	
	// Prevent 2 threads from modifying the same user
	std::lock_guard<std::mutex> lk(users[username].mtx);
	
	User &user = users[username];
	
	for (auto &element : values.items()) {
		PropertyValue &property = user.properties[element.key()];
		// Only update this property if the timestamp is newer than the previous one
		if (property.last_updated_timestamp < timestamp) {
			property.value = element.value();
			property.last_updated_timestamp = timestamp;
		}
	}
}

void process_input(int thread_id) {
	// Consume every 20th line of input, starting with the `thread_id`-th line.
	// Thread 0 will process lines 0, 20, 40, ...
	// Thread 1 will process lines 1, 21, 41, ...
	// Thread 2 will process lines 2, 22, 42, ...
	for (int i = thread_id; i < input_file.size(); i += 20) {
		json &command = input_file[i];
		update(command["user"], command["timestamp"], command["values"]);
	}
}



// ====================== MAIN ======================

int main(int argc, char *argv[]) {
	
	// Make sure the command line arguments are well formed
	if (!(argc == 3 && std::string(argv[1]) == "-i")) {
		cout << "Invalid input parameters" << endl;
		exit(1);
	}
	
	// Read from the input file specified in the command line arguments
	std::ifstream input_stream(argv[2]);
	
	// Check that the input file can be opened
	if (!input_stream.is_open()) {
		cout << "Input file not found!" << endl;
		exit(1);
	}
	
	auto start_time = std::chrono::steady_clock::now();
	
	// Read input
	for (std::string line; std::getline(input_stream, line);)
		input_file.push_back(json::parse(line));
	input_stream.close();
	
	// Populate the user database with all the users' names.
	// If we try to modify the std::map while processing the updates, there could be a segmentation fault.
	// (Alternative solution: make updating the map thread-safe.)
	for (auto &command : input_file)
		User &u = users[command["user"]];
	
	// Use 20 threads
	std::vector<std::thread> threads;
	for (int i = 0; i < 20; i++)
		threads.emplace_back(process_input, i);
	
	// Wait for all threads to complete
	for (auto &thread : threads)
		thread.join();
	
	// Output state
	json state;
	for (auto &[username, user] : users) {
		json user_state;
		for (auto &[property_name, property] : user.properties)
			user_state[property_name] = property.value;
		state[username] = user_state;
	}
	cout << state.dump(4) << endl;
	
	double time_taken = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::steady_clock::now() - start_time).count();
	// cout << "Time taken: " << time_taken << " seconds" << endl;
}
