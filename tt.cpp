//#include <iostream>
//#include <optional>
//#include <string_view>
//
//class User {
//    std::string name;
//    std::optional<std::string> nickName;
//    std::optional<int> age;
//public:
//    User(std::string_view _name, std::optional<std::string_view> _nickName, std::optional<int> _age)
//            : name(_name), nickName(_nickName), age(_age) {
//    };
//
//    friend std::ostream &operator<<(std::ostream &os, const User &user);
//};
//
//std::ostream &operator<<(std::ostream &os, const User &user) {
//    os << user.name << ".";
//    if (user.nickName) {
//        os << *user.nickName << ".";
//    }
//    if (user.age) {
//        os << *user.age << ".";
//    }
//
//    return os;
//};
//
//int main() {
//    User u0("Lisa", "crazeHorse", 24);
//    User u1("baby", std::nullopt, 88);
//    User u2("baby", std::nullopt, std::nullopt);
//
//    std::cout << u0 << "\n"
//              << u1 << "\n"
//              << u2 << "\n";
//    return 0;
//}
