

[English](#english) | [Русский](https://github.com/The-IDK-Project/Messenger/blob/master/docs/ARCHITECTURE_RU.md)

<a name="english"></a>
# Architecture Overview

#System Overview

Unified Messenger is a multi-protocol chat client built with a modular, layered architecture that separates concerns while providing a unified user experience across different chat protocols.

#High-Level Architecture
```
┌─────────────────────────────────────────────────────────────┐
│ Presentation Layer                                          │
├─────────────────────────────────────────────────────────────┤
│ TUI (ncurses) │ GUI (Qt) │ Web UI*                          │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Application Layer                                           │
├─────────────────────────────────────────────────────────────┤
│ UnifiedMessenger │ SessionManager │ NotificationManager     │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Business Logic Layer                                        │
├─────────────────────────────────────────────────────────────┤
│ ProtocolManager │ MessageRouter │ SyncEngine                │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Protocol Abstraction Layer                                  │
├─────────────────────────────────────────────────────────────┤
│ MatrixHandler │ IRCHandler │ TelegramHandler                │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Data Access Layer │
├─────────────────────────────────────────────────────────────┤
│ DatabaseManager + SQLite Storage                            │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Network Layer                                               │
├─────────────────────────────────────────────────────────────┤
│ HTTP Client │ WebSocket Client │ IRC Socket │ TDLib         │
└─────────────────────────────────────────────────────────────┘
```

##Core Components

#Data Models (`core/`)

**Message** - Universal message representation:
```
class Message {
    std::string id;           // Cross-protocol unique ID
    std::string content;      // Message text/content
    std::string sender_id;    // Sender identifier
    std::string sender_name;  // Display name
    std::string room_id;      // Room/channel identifier
    std::string protocol;     // "matrix", "irc", "telegram"
    MessageType type;         // TEXT, IMAGE, FILE, SYSTEM
    MessageStatus status;     // SENDING, SENT, DELIVERED, READ
    std::chrono::system_clock::time_point timestamp;
    std::string reply_to_id;  // For thread replies
};
```
#User - Unified user representation
```
class User {
    std::string id;           // Protocol-specific user ID
    std::string username;     // Login name
    std::string display_name; // Display name
    std::vector<std::string> protocols; // Supported protocols
    std::string avatar_url;   // Profile picture
};
```
#ChatRoom - Room/channel abstraction:
```
class ChatRoom {
    std::string id;           // Room identifier
    std::string name;         // Display name
    std::string protocol;     // Source protocol
    std::vector<std::string> participants; // Member list
    RoomType type;            // DIRECT, GROUP, CHANNEL
    bool is_encrypted;        // E2E encryption status
};
```
#Database Layer (database/)

```
-- Messages table with protocol support
CREATE TABLE messages (
    id TEXT PRIMARY KEY,
    content TEXT NOT NULL,
    sender_id TEXT NOT NULL,
    sender_name TEXT NOT NULL,
    room_id TEXT NOT NULL,
    protocol TEXT NOT NULL,
    type INTEGER DEFAULT 0,
    status INTEGER DEFAULT 0,
    timestamp INTEGER NOT NULL,
    reply_to_id TEXT,
    metadata TEXT -- JSON for protocol-specific data
);

-- Users table
CREATE TABLE users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    display_name TEXT,
    protocols TEXT,
    avatar_url TEXT,
    last_seen INTEGER
);

-- Rooms table
CREATE TABLE rooms (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    protocol TEXT NOT NULL,
    participants TEXT,
    type INTEGER DEFAULT 0,
    is_encrypted INTEGER DEFAULT 0,
    last_message_id TEXT
);

-- Sessions and credentials (encrypted)
CREATE TABLE sessions (
    protocol TEXT PRIMARY KEY,
    access_token TEXT,
    user_id TEXT,
    server_config TEXT,
    last_sync_token TEXT
);
```
DatabaseManager provides:

    Connection pooling and thread safety

    Migration management

    Efficient querying with indexes

    Backup and recovery mechanisms

#Protocol Abstraction Layer (protocols/)

###ProtocolHandler Interface:
```
class ProtocolHandler {
public:
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    // Message operations
    virtual bool sendMessage(const std::string& room_id, 
                           const std::string& message) = 0;
    virtual bool sendFile(const std::string& room_id,
                         const std::string& file_path) = 0;
    
    // Room management
    virtual bool joinRoom(const std::string& room_id) = 0;
    virtual bool leaveRoom(const std::string& room_id) = 0;
    virtual std::vector<ChatRoom> getRooms() = 0;
    
    // User management
    virtual User getCurrentUser() = 0;
    virtual std::vector<User> getRoomUsers(const std::string& room_id) = 0;
    
    // Synchronization
    virtual void sync() = 0;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    // Protocol info
    virtual std::string getProtocolName() const = 0;
    virtual ProtocolCapabilities getCapabilities() const = 0;
};
```
#Protocol Implementations

###MatrixHandler:

    Uses Matrix Client-Server API

    Implements end-to-end encryption

    Supports rooms, spaces, and rich media

    WebSocket for real-time updates

###IRCHandler:

    Raw TCP socket connection

    IRC protocol parsing

    Channel and user management

    CTCP and DCC support

###TelegramHandler:

    TDLib integration

    Secret chats support

    File transfer capabilities

    Phone number authentication

#Network Layer (network/)

###HttpClient - REST API client:

    SSL/TLS support

    JSON request/response handling

    Authentication management

    Rate limiting and retry logic

###WebSocketClient - Real-time communication:

    Matrix sync API

    Event-based messaging

    Connection management

###IRCConnection - IRC protocol handler:

    Message parsing and serialization

    Connection state management

    CTCP handling

#Application Layer (app/)

###UnifiedMessenger - Main coordinator:

```
class UnifiedMessenger {
    // Protocol management
    std::map<std::string, std::unique_ptr<ProtocolHandler>> protocols_;
    
    // Data management
    std::unique_ptr<DatabaseManager> database_;
    
    // UI coordination
    std::function<void(const Message&)> message_callback_;
    
public:
    bool initialize();
    void shutdown();
    void sendMessage(const std::string& protocol, 
                    const std::string& room_id,
                    const std::string& message);
    std::vector<Message> getUnifiedInbox();
};
```

###SessionManager - User session handling:

    Authentication state management

    Token refresh and validation

    Multi-account support

###NotificationManager - Cross-platform notifications:

    Desktop notifications

    Sound alerts

    Badge counters

#UI Layer (ui/)

###Interface Abstraction:

```
class Interface {
public:
    virtual void run() = 0;
    virtual void displayMessage(const Message& message) = 0;
    virtual void updateRoomList(const std::vector<ChatRoom>& rooms) = 0;
    virtual void setInputHandler(InputHandler handler) = 0;
};
```

###TUI Implementation (ncurses):

    Terminal-based interface

    Multi-pane layout

    Vim-like keybindings

    Color themes support

###GUI Implementation (Qt):

    Modern desktop interface

    Drag-and-drop file transfer

    Rich text formatting

    System tray integration

#Data Flow

###Message Reception:

```
Protocol Server → Network Layer → ProtocolHandler → 
Message Conversion → Database Storage → UI Update
```

###Message Sending:

```
UI Input → UnifiedMessenger → ProtocolHandler → 
Network Layer → Protocol Server → Status Update
```

###Synchronization:
```
Sync Timer → ProtocolHandler.sync() → 
New Messages → Database → UI Notification
```

#Concurrency Model

```
Main Thread: UI Rendering and Input
│
├── Database Thread: SQLite operations
│
├── Network Threads: One per protocol
│   ├── Matrix: HTTP + WebSocket
│   ├── IRC: TCP socket
│   └── Telegram: TDLib events
│
└── Worker Threads: File processing, crypto
```

#Security Architecture

    Credential Storage: Encrypted SQLite with system keychain integration

    Network Security: TLS for all external communications

    Data Protection: Encryption at rest for sensitive data

    Input Validation: Comprehensive sanitization of all input

#Configuration Management

###Hierarchical Configuration:

```
; System-wide defaults
/etc/unified-messenger/default.conf

; User preferences
~/.config/unified-messenger/config.conf

; Protocol-specific
~/.config/unified-messenger/matrix.conf
```

#Extension Points

###Adding New Protocols:

    Implement ProtocolHandler interface

    Register in ProtocolFactory

    Add configuration schema

    Update UI components

###Custom Storage Backends:

    Implement StorageInterface

    Configure in database layer

    Handle migrations

###Plugin System:

    Loadable modules for additional features

    Hook system for custom processing

    Theme and widget extensions

###Performance Considerations
    Database: Indexed queries, connection pooling

    Memory: Object pooling, efficient data structures

    Network: Connection reuse, compression

    UI: Virtual scrolling, lazy loading
