#include "UserStore.h"

using namespace std;

UserStore UserStore::INSTANCE = UserStore();

UserStore::UserStore()
{
    populateMap();
}

UserStore::~UserStore()
{
}

const User *UserStore::findUser(const std::string username)
{
    auto c = users.find(username);

    if (c != users.end())
    {
        return &(c->second);
    }
    return NULL;
}

void UserStore::populateMap()
{
    users["user0"] = User{"user0", "password0"};
    users["user1"] = User{"user1", "password1"};
    users["user2"] = User{"user2", "password2"};
    users["user3"] = User{"user3", "password3"};
    users["user4"] = User{"user4", "password4"};
}