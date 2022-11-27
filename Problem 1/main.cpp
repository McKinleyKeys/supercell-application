
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include "nlohmann/json.hpp"

using std::cin;
using std::cout;
using std::endl;
using json = nlohmann::json;



// ====================== DATABASE ======================

struct PropertyValue {
	std::string value;
	int last_updated_timestamp;  // The timestamp when this value was last updated
};
struct User {
	std::set<std::string> friends;
	std::map<std::string, PropertyValue> properties;
};

std::map<std::string, User> users;



// ====================== DATABASE UPDATE FUNCTIONS ======================

void make_friends(const std::string &user1, const std::string &user2) {
	users[user1].friends.insert(user2);
	users[user2].friends.insert(user1);
}
void unmake_friends(const std::string &user1, const std::string &user2) {
	users[user1].friends.erase(user2);
	users[user2].friends.erase(user1);
}
void update(const std::string &username, int timestamp, const json &values) {
	
	User &user = users[username];
	
	// Store a list of all values that are actually changed
	json updated_values;
	
	for (auto &element : values.items()) {
		PropertyValue &property = user.properties[element.key()];
		// Only update this property if the timestamp is newer than the previous one
		if (property.last_updated_timestamp < timestamp) {
			property.value = element.value();
			property.last_updated_timestamp = timestamp;
			updated_values[element.key()] = element.value();
		}
	}
	
	// Broadcast any updates
	if (updated_values.size() && user.friends.size()) {
		
		json friends;
		for (const std::string &fr : user.friends)
			friends.push_back(fr);
		
		json message;
		message["broadcast"] = friends;
		message["user"] = username;
		message["timestamp"] = timestamp;
		message["values"] = updated_values;
		
		cout << message.dump() << endl;
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
	std::ifstream input_file(argv[2]);
	
	// Check that the input file can be opened
	if (!input_file.is_open()) {
		cout << "Input file not found!" << endl;
		exit(1);
	}
	
	// Read JSON lines from file
	for (std::string line; std::getline(input_file, line);) {
		
		json command = json::parse(line);
		
		if (command["type"] == "make_friends")
			make_friends(command["user1"], command["user2"]);
		else if (command["type"] == "del_friends")
			unmake_friends(command["user1"], command["user2"]);
		else if (command["type"] == "update")
			update(command["user"], command["timestamp"], command["values"]);
	}
	
	input_file.close();
}
