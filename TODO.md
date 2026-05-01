# TODO List

This file contains a list of suggested improvements and potential bugs to address in the Verita project.

## High Priority (Potential Bugs)

*   **[ ] Fix dangling reference in UI callbacks:**
    *   **File:** `scr/main.cpp`
    *   **Issue:** In `setup_callbacks`, the `ui` object is captured by reference in several lambdas. However, the `ui` unique_ptr is owned by the `run_application` function and will be destroyed when that function exits, leaving dangling references in the callbacks, which are likely called asynchronously.
    *   **Suggestion:** The `Interface` object's lifetime needs to be managed carefully. It should probably be owned by the `Verita` application object or live for the entire duration of `main`. Consider passing `app` to `create_interface` and having the `Verita` class hold the `std::unique_ptr<Interface>`.

*   **[ ] Manage `QApplication` lifetime correctly:**
    *   **File:** `scr/main.cpp`
    *   **Issue:** The `qapp` unique_ptr goes out of scope at the end of `run_application`, but the Qt event loop (`ui->run()`) needs the `QApplication` instance to exist for the entire lifetime of the GUI. The current code might work by chance, but it's not robust.
    *   **Suggestion:** The `QApplication` object should be created in `main` and its lifetime should enclose the entire application logic.

## Medium Priority (Improvements)

*   **[ ] Improve command-line argument parsing:**
    *   **File:** `scr/main.cpp`
    *   **Issue:** The argument parser ignores unknown arguments. This can hide typos and make debugging difficult.
    *   **Suggestion:** Report unknown arguments to the user as an error.

*   **[ ] Make protocol selection dynamic:**
    *   **File:** `scr/main.cpp`
    *   **Issue:** In `setup_callbacks`, the code always uses the first available protocol (`protocols[0]`) for sending messages and making calls.
    *   **Suggestion:** The UI should allow the user to select which protocol to use for an action, or the application should have a more sophisticated way of choosing the protocol based on the active room or user.

*   **[ ] Cross-platform home directory detection:**
    *   **File:** `scr/main.cpp`
    *   **Issue:** `setup_logging` uses `getenv("HOME")` to find the user's home directory. This is not portable to Windows.
    *   **Suggestion:** Use a C++17 `std::filesystem::path` or a library like Qt's `QStandardPaths` to find the home directory in a cross-platform way.

*   **[ ] Refine build script:**
    *   **File:** `build.sh`
    *   **Issue:** The script has a hardcoded, fragile way of finding and copying DLLs. It assumes a specific directory layout for MinGW and Qt.
    *   **Suggestion:** Use `windres` to embed a manifest. For deployment, consider using `windeployqt` for Qt dependencies and a more robust method for finding the MinGW runtime DLLs (e.g., by querying the compiler).

## Low Priority (Code Style & Cleanup)

*   **[ ] Unused lambda parameter:**
    *   **File:** `scr/main.cpp`, function `setup_callbacks`
    *   **Issue:** The lambda for `app.set_room_callback` has an unused `room` parameter.
    *   **Suggestion:** Mark it as unused to avoid compiler warnings: `[](const ChatRoom& /*room*/)`.

*   **[ ] Review includes:**
    *   **Issue:** Some headers might be included in `.cpp` files when they could be in `.h` files, or vice-versa. A general review could improve build times and code organization.
