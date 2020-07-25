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
};


