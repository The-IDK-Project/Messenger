
<a name="русский"></a>

# Обзор архитектуры
### Обзор системы
#### Unified Messenger - это мультипротокольный мессенджер, построенный по модульной, многоуровневой архитектуре, которая разделяет ответственность компонентов, обеспечивая единый пользовательский опыт для разных протоколов чатов.

### Высокоуровневая архитектура
```
┌─────────────────────────────────────────────────────────────┐
│                    Слой представления                       │
├─────────────────────────────────────────────────────────────┤
│  TUI (ncurses)  │      GUI (Qt)       │     Web UI*         │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Прикладной слой                           │
├─────────────────────────────────────────────────────────────┤
│  UnifiedMessenger │ SessionManager │ NotificationManager    │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                    Слой бизнес-логики                       │
├─────────────────────────────────────────────────────────────┤
│  ProtocolManager  │  MessageRouter  │   SyncEngine          │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                    Слой абстракции протоколов               │
├─────────────────────────────────────────────────────────────┤
│  MatrixHandler   │   IRCHandler    │  TelegramHandler       │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                    Слой доступа к данным                    │
├─────────────────────────────────────────────────────────────┤
│          DatabaseManager + SQLite Storage                   │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                    Сетьвой слой                             │
├─────────────────────────────────────────────────────────────┤
│  HTTP Client  │ WebSocket Client │  IRC Socket  │  TDLib    │
└─────────────────────────────────────────────────────────────┘
```
## Основные компоненты
# Модели данных (core/)
### Message - Универсальное представление сообщения:

```
class Message {
    std::string id;           // Кросс-протокольный уникальный ID
    std::string content;      // Текст/содержимое сообщения
    std::string sender_id;    // Идентификатор отправителя
    std::string sender_name;  // Отображаемое имя
    std::string room_id;      // Идентификатор комнаты/канала
    std::string protocol;     // "matrix", "irc", "telegram"
    MessageType type;         // TEXT, IMAGE, FILE, SYSTEM
    MessageStatus status;     // SENDING, SENT, DELIVERED, READ
    std::chrono::system_clock::time_point timestamp;
    std::string reply_to_id;  // Для ответов в тредах
};
```
### User - Унифицированное представление пользователя:
```
class User {
    std::string id;           // Протокол-специфичный ID пользователя
    std::string username;     // Логин
    std::string display_name; // Отображаемое имя
    std::vector<std::string> protocols; // Поддерживаемые протоколы
    std::string avatar_url;   // Аватар
};
```
### ChatRoom - Абстракция комнаты/канала:

```
class ChatRoom {
    std::string id;           // Идентификатор комнаты
    std::string name;         // Отображаемое имя
    std::string protocol;     // Исходный протокол
    std::vector<std::string> participants; // Список участников
    RoomType type;            // DIRECT, GROUP, CHANNEL
    bool is_encrypted;        // Статус сквозного шифрования
};
```
# Слой базы данных (database/)

### Схема SQLite:

```
-- Таблица сообщений с поддержкой протоколов
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
    metadata TEXT -- JSON для протокол-специфичных данных
);

-- Таблица пользователей
CREATE TABLE users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    display_name TEXT,
    protocols TEXT,
    avatar_url TEXT,
    last_seen INTEGER
);

-- Таблица комнат
CREATE TABLE rooms (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    protocol TEXT NOT NULL,
    participants TEXT,
    type INTEGER DEFAULT 0,
    is_encrypted INTEGER DEFAULT 0,
    last_message_id TEXT
);

-- Сессии и учетные данные (зашифрованы)
CREATE TABLE sessions (
    protocol TEXT PRIMARY KEY,
    access_token TEXT,
    user_id TEXT,
    server_config TEXT,
    last_sync_token TEXT
);
```
### DatabaseManager предоставляет:

Пул соединений и потокобезопасность

Управление миграциями

Эффективные запросы с индексами

Механизмы резервного копирования и восстановления

# Слой абстракции протоколов (protocols/)
### Интерфейс ProtocolHandler:
```
class ProtocolHandler {
public:
    // Управление подключением
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    // Операции с сообщениями
    virtual bool sendMessage(const std::string& room_id, 
                           const std::string& message) = 0;
    virtual bool sendFile(const std::string& room_id,
                         const std::string& file_path) = 0;
    
    // Управление комнатами
    virtual bool joinRoom(const std::string& room_id) = 0;
    virtual bool leaveRoom(const std::string& room_id) = 0;
    virtual std::vector<ChatRoom> getRooms() = 0;
    
    // Управление пользователями
    virtual User getCurrentUser() = 0;
    virtual std::vector<User> getRoomUsers(const std::string& room_id) = 0;
    
    // Синхронизация
    virtual void sync() = 0;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    // Информация о протоколе
    virtual std::string getProtocolName() const = 0;
    virtual ProtocolCapabilities getCapabilities() const = 0;
};
```
# Реализации протоколов
### MatrixHandler:

Использует Matrix Client-Server API

Реализует сквозное шифрование

Поддерживает комнаты, пространства и медиа

WebSocket для обновлений в реальном времени

### IRCHandler:

Сырое TCP socket соединение

Парсинг протокола IRC

Управление каналами и пользователями

Поддержка CTCP и DCC

### TelegramHandler:

Интеграция с TDLib

Поддержка секретных чатов

Возможности передачи файлов

Аутентификация по номеру телефона

# Сетевой слой (network/)
### HttpClient - REST API клиент:

Поддержка SSL/TLS

Обработка JSON запросов/ответов

Управление аутентификацией

Ограничение частоты и логика повторов

### WebSocketClient - Коммуникация в реальном времени:

Matrix sync API

Событийная модель сообщений

Управление соединением

### IRCConnection - Обработчик протокола IRC:

Парсинг и сериализация сообщений

Управление состоянием соединения

Обработка CTCP

# Прикладной слой (app/)
### UnifiedMessenger - Главный координатор:

```
class UnifiedMessenger {
    // Управление протоколами
    std::map<std::string, std::unique_ptr<ProtocolHandler>> protocols_;
    
    // Управление данными
    std::unique_ptr<DatabaseManager> database_;
    
    // Координация UI
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
### SessionManager - Управление пользовательскими сессиями:

Управление состоянием аутентификации

Обновление и валидация токенов

Поддержка нескольких аккаунтов

### NotificationManager - Кроссплатформенные уведомления:

Десктопные уведомления

Звуковые оповещения

Счетчики бейджей

# Слой пользовательского интерфейса (ui/)

### Абстракция интерфейса:

```
class Interface {
public:
    virtual void run() = 0;
    virtual void displayMessage(const Message& message) = 0;
    virtual void updateRoomList(const std::vector<ChatRoom>& rooms) = 0;
    virtual void setInputHandler(InputHandler handler) = 0;
};
```
### Реализация TUI (ncurses):

Терминальный интерфейс

Многопанельный layout

Vim-подобные горячие клавиши

Поддержка цветовых тем

### Реализация GUI (Qt):

Современный десктопный интерфейс

Перетаскивание файлов

Форматирование богатого текста

Интеграция с системным треем

# Поток данных

### Получение сообщения:
```
Сервер протокола → Сетевой слой → ProtocolHandler → 
Конвертация сообщения → Сохранение в БД → Обновление UI
```
### Отправка сообщения:
```
Ввод в UI → UnifiedMessenger → ProtocolHandler → 
Сетевой слой → Сервер протокола → Обновление статуса
```
### Синхронизация:
```
Таймер синхронизации → ProtocolHandler.sync() → 
Новые сообщения → База данных → Уведомление UI
```
### Модель параллелизма

```
Главный поток: Отрисовка UI и ввод
│
├── Поток базы данных: Операции SQLite
│
├── Сетевые потоки: По одному на протокол
│   ├── Matrix: HTTP + WebSocket
│   ├── IRC: TCP socket
│   └── Telegram: События TDLib
│
└── Рабочие потоки: Обработка файлов, криптография
```
### Архитектура безопасности

Хранение учетных данных: Зашифрованная SQLite с интеграцией системного keychain

Сетевая безопасность: TLS для всех внешних коммуникаций

Защита данных: Шифрование неактивных чувствительных данных

Валидация ввода: Комплексная санитизация всех входных данных

# Управление конфигурацией

### Иерархическая конфигурация:

```
; Системные значения по умолчанию
/etc/unified-messenger/default.conf

; Пользовательские предпочтения
~/.config/unified-messenger/config.conf

; Протокол-специфичные настройки
~/.config/unified-messenger/matrix.conf
```
# Точки расширения

### Добавление новых протоколов:

Реализовать интерфейс ProtocolHandler

Зарегистрировать в ProtocolFactory

Добавить схему конфигурации

Обновить UI компоненты

### Пользовательские бэкенды хранения:

Реализовать StorageInterface

Настроить в слое базы данных

Обработать миграции

### Система плагинов:

Загружаемые модули для дополнительных функций

Система хуков для пользовательской обработки

Расширения тем и виджетов

### Соображения производительности

База данных: Индексированные запросы, пул соединений

Память: Пул объектов, эффективные структуры данных

Сеть: Повторное использование соединений, сжатие

UI: Виртуальная прокрутка, ленивая загрузка
