#pragma once

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

class SociDb {
public:
	soci::session sql;

	void ConnectToDB() {
		sql.open(soci::postgresql, "dbname=chatserverdb user=postgres password=1234 port=5432 host=localhost");
	}
	bool GetUser(std::string username_, std::string password_) {
		std::string username;
		sql << "select username from accounts where username = :name and password = :pass;",
			soci::into(username), soci::use(username_), soci::use(password_);

		if (username != "") {
			return true;
		}
		else {
			return false;
		}
	}
	//returns true if can add user
	bool addUser(std::string username, std::string password) {
		bool exists = checkUserExists(username, password);
		if (!exists) {
			if (username == "" || password == "") {
				return false;
			}
			sql << "insert into accounts(username, password) values(:name, :pass)",
				soci::use(username), soci::use(password);
			return true;
		}
		return false;
	}
	//return true if user exists. Else returns false
	bool checkUserExists(std::string username, std::string password) {
		std::string user;
		sql << "select username from accounts where username = :name and password = :pass;",
			soci::into(user), soci::use(username), soci::use(password);
		if (user == "") {
			return false;
		}
		else {
			return true;
		}
	}
};


