#include <json.hpp>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
    json j;
    j["hello"] = "world";

    std::string s = j.dump();
    printf("%s\n", s.c_str());

    return 0;
}
