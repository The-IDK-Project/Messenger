#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <functional>

struct Migration {
    int version;
    std::string description;
    std::function<bool(sqlite3*)> up;
    std::function<bool(sqlite3*)> down;
};

class Migrations {
public:
    static bool run_migrations(sqlite3* db);
    static bool migrate_to_version(sqlite3* db, int target_version);
    static bool rollback_to_version(sqlite3* db, int target_version);
    static int get_current_version(sqlite3* db);
    static int get_latest_version() { return migrations_.size(); }
    static std::vector<std::string> get_migration_history(sqlite3* db);

    static bool migration_v1(sqlite3* db);  // Initial schema
    static bool migration_v2(sqlite3* db);  // Add metadata field
    static bool migration_v3(sqlite3* db);  // Add session management

    static bool rollback_v1(sqlite3* db);
    static bool rollback_v2(sqlite3* db);
    static bool rollback_v3(sqlite3* db);

private:
    static void initialize_migrations();
    static bool create_migrations_table(sqlite3* db);
    static bool record_migration(sqlite3* db, int version, bool applied);

    static std::vector<Migration> migrations_;
    static bool initialized_;
};