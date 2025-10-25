# Contributing Guide
### Quick Start

Fork the repository

Create feature branch: git checkout -b The-IDK-Project/super-duper-giggle

Make changes and test: cd build && make && make test

Commit: git commit -m 'feat: add amazing feature'

Push: git push origin The-IDK-Project/super-duper-giggle

Open Pull Request

# Code Standards
C++17 with Google Style Guide

4 spaces indentation

camelCase for methods, snake_case for variables

English comments and commit messages

# Commit Message Format
```
type(scope): description

feat(matrix): add file upload support
fix(irc): handle connection timeout
docs: update api documentation
```
Types: ```feat```, ```fix```, ```docs```, ```style```, ```refactor```, ```test```, ```chore```

# Testing
```
# Run all tests
cd build && ctest

# Run specific test
./tests/unit/test_database

# Code coverage
cmake .. -DBUILD_COVERAGE=ON

make coverage
```
Pull Request Process

Update documentation if needed

Add tests for new functionality

Ensure all tests pass

Request review from maintainers

# Development Setup
```
# Debug build with tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# With all features
cmake .. -DBUILD_GUI=ON -DENABLE_TELEGRAM=ON
```
Need Help?

Open an Issue for bugs

Use Discussions for questions

Join Matrix room: #TheIDKTeam:matrix.org
