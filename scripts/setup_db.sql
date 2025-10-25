BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS messages (
                                        id TEXT PRIMARY KEY,
                                        content TEXT NOT NULL,
                                        sender_id TEXT NOT NULL,
                                        sender_name TEXT NOT NULL,
                                        room_id TEXT NOT NULL,
                                        protocol TEXT NOT NULL CHECK (protocol IN ('matrix', 'irc', 'telegram')),
    type INTEGER DEFAULT 0,
    status INTEGER DEFAULT 0,
    timestamp INTEGER NOT NULL,
    reply_to_id TEXT,
    metadata TEXT,
    created_at INTEGER DEFAULT (strftime('%s','now'))
    );

CREATE TABLE IF NOT EXISTS users (
                                     id TEXT PRIMARY KEY,
                                     username TEXT NOT NULL,
                                     display_name TEXT,
                                     protocols TEXT,
                                     avatar_url TEXT,
                                     last_seen INTEGER,
                                     created_at INTEGER DEFAULT (strftime('%s','now'))
    );

CREATE TABLE IF NOT EXISTS rooms (
                                     id TEXT PRIMARY KEY,
                                     name TEXT NOT NULL,
                                     protocol TEXT NOT NULL CHECK (protocol IN ('matrix', 'irc', 'telegram')),
    participants TEXT,
    type INTEGER DEFAULT 0,
    is_encrypted INTEGER DEFAULT 0,
    last_message_id TEXT,
    last_activity INTEGER,
    created_at INTEGER DEFAULT (strftime('%s','now'))
    );

CREATE TABLE IF NOT EXISTS sessions (
                                        protocol TEXT PRIMARY KEY CHECK (protocol IN ('matrix', 'irc', 'telegram')),
    access_token TEXT,
    user_id TEXT,
    server_config TEXT,
    last_sync_token TEXT,
    is_connected INTEGER DEFAULT 0,
    last_connected INTEGER,
    created_at INTEGER DEFAULT (strftime('%s','now'))
    );

CREATE TABLE IF NOT EXISTS settings (
                                        key TEXT PRIMARY KEY,
                                        value TEXT NOT NULL,
                                        updated_at INTEGER DEFAULT (strftime('%s','now'))
    );

CREATE INDEX IF NOT EXISTS idx_messages_room_timestamp ON messages(room_id, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_messages_protocol ON messages(protocol);
CREATE INDEX IF NOT EXISTS idx_messages_sender ON messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_rooms_protocol ON rooms(protocol);
CREATE INDEX IF NOT EXISTS idx_rooms_activity ON rooms(last_activity DESC);
CREATE INDEX IF NOT EXISTS idx_users_protocols ON users(protocols);

INSERT OR IGNORE INTO settings (key, value) VALUES
    ('db_version', '1.0'),
    ('ui_theme', 'dark'),
    ('notifications_enabled', 'true'),
    ('auto_connect', 'false'),
    ('message_history_days', '30');

COMMIT;

SELECT 'Database schema created successfully' as status;