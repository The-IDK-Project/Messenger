
INSERT INTO messages (id, content, sender_id, sender_name, room_id, protocol, type, status, timestamp) VALUES
                                                                                                           ('msg1', 'Hello everyone!', 'user1', 'Alice', 'room1', 'matrix', 0, 2, 1672531200),
                                                                                                           ('msg2', 'How are you?', 'user2', 'Bob', 'room1', 'matrix', 0, 2, 1672531260),
                                                                                                           ('msg3', 'Testing 123', 'user3', 'Charlie', 'room2', 'irc', 0, 2, 1672531320),
                                                                                                           ('msg4', 'System message', 'system', 'System', 'room1', 'matrix', 3, 2, 1672531380);

INSERT INTO users (id, username, display_name, protocols, last_seen) VALUES
                                                                         ('user1', 'alice', 'Alice', '["matrix", "irc"]', 1672531200),
                                                                         ('user2', 'bob', 'Bob', '["matrix"]', 1672531200),
                                                                         ('user3', 'charlie', 'Charlie', '["irc"]', 1672531200),
                                                                         ('user4', 'dave', 'Dave', '["matrix", "telegram"]', 1672531200);

INSERT INTO rooms (id, name, protocol, participants, type, last_activity) VALUES
                                                                              ('room1', 'General Chat', 'matrix', '["user1", "user2", "user4"]', 1, 1672531380),
                                                                              ('room2', '#testchannel', 'irc', '["user1", "user3"]', 2, 1672531320),
                                                                              ('room3', 'Private Chat', 'matrix', '["user1", "user2"]', 0, 1672531200);

INSERT INTO sessions (protocol, access_token, user_id, is_connected, last_connected) VALUES
                                                                                         ('matrix', 'fake_token_123', 'user1', 1, 1672531200),
                                                                                         ('irc', '', 'user1', 0, 1672530000);

INSERT INTO settings (key, value) VALUES
                                      ('ui_theme', 'dark'),
                                      ('notifications_enabled', 'true'),
                                      ('auto_connect', 'false');