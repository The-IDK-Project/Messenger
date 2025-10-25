# Руководство по участию
### Быстрый старт

Форк репозитория

Создание ветки функциональности: git checkout -b The-IDK-Project/super-duper-giggle

Внесение изменений и тестирование: cd build && make && make test

Коммит: git commit -m 'feat: add amazing feature'

Пуш: git push origin The-IDK-Project/super-duper-giggle

Открытие запроса на извлечение

# Стандарты кода
C++17 с Google Style Guide

Отступ 4 пробела

camelCase для методов, snake_case для переменных

Комментарии и сообщения коммита на английском языке

# Формат сообщения коммита
```
type(scope): description

feat(matrix): добавить поддержку загрузки файлов
fix(irc): обрабатывать тайм-аут соединения
docs: обновить документацию API
```
Типы: ```feat```, ```fix```, ```docs```, ```style```, ```refactor```, ```test```, ```chore```

# Тестирование
```
# Выполнение всех тестов
cd build && ctest

# Выполнение определённого теста
./tests/unit/test_database

# Покрытие кода
cmake .. -DBUILD_COVERAGE=ON

make cover
```
Процесс запроса на включение изменений

При необходимости обновите документацию

Добавьте тесты для новой функциональности

Убедитесь, что все тесты пройдены

Запросите проверку у мейнтейнеров

# Настройка разработки
```
# Отладка сборки с помощью тестов
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Со всеми функциями
cmake .. -DBUILD_GUI=ON -DENABLE_TELEGRAM=ON
```
Нужна помощь?

Открыть запрос об ошибках

Используйте обсуждения для вопросов

Присоединяйтесь к комнате Matrix: #TheIDKTeam:matrix.org