#pragma once

#include <unordered_map>
#include <string>

struct User
{
    std::string username;
    std::string password;
};

class UserStore
{
  public:
    static UserStore INSTANCE;

    UserStore();
    ~UserStore();
    const User *findUser(const std::string username);

  private:
    std::unordered_map<std::string, User> users;

    void populateMap();
};