#include "json.hpp"
#include <iostream>

using json = nlohmann::json;
int main(int argc, char ** argv){
	
	json j;
	j["pi"] = "hello world";
	j["no"] = "dont print";
	std::string s = j["pi"].dump();
	std::cout << s <<std::endl;
	return 0;
}
