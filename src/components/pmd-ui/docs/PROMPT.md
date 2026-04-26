# pmd-ui Specification

pmd-ui is the user interface for the pmd service.

"pmd" stands for "PE Malware Detection".

## UI

### Main View

- This is the primary interface where users can interact with the application.
  It has a table view to display the list of malware detected.
  There are three columns: File Name, Detection Time, Label (malware or benign), Severity (0.0 - 1.0).
  Users can sort the table by clicking on the column headers.

- Clicking an entry in the table opens a detailed view of the selected malware,
  showing additional information:
    - Detection Time
    - File Path
    - File ID
    - Volume Serial Number
    - Label
    - Severity

- The main view has a menu bar with the following options:
    - File
      - Exit: Closes the application.
    - Help
      - About: Displays information about the application:
        - Version: 1.0.0
        - Author: Vu Tung Lam

- The main view also has a status bar at the bottom to show messages
  showing status, either being:
  - Last updated at `<time>` ; or
  - Could not connect to pmd-driver.

### Notification

- The application shows a notification popup when new malware is detected.
- The notification displays the file name and severity of the detected malware.
- The notification disappears after a few seconds or when clicked by the user.
- Clicking the notification opens the main view of the application.

## Implementation Details

- The application is written in C++, using Windows API for creating windows,
    menus, and handling events.

- Build tool: CMake.

- The program should have a worker thread to periodically fetch new
    malware data from the pmd-driver, via the interface defined in
    `driver/include/pmd_driver/communication.h`.

    Notably, the worker thread must first call `OpenDriverDevice()` to
    obtain a valid device handle. Then, it should use `GetBlockOperation()`
    to retrieve block operation data from the driver, periodically,
    ideally every 1 second.

    The fetched data should then be displayed in the main view's table.

    Upon clean up, the worker thread must call `CloseDriverDevice()` to
    properly close the device handle.

- The main thread should notify the worker thread to stop and wait for its
    termination before exiting the application, preferably via
    a Windows Event object.

- `GetBlockOperation()` outputs data into a `BLOCK_OPERATION_DTO` structure,
    which is defined in `driver/include/pmd_driver/dto.h`. This structure
    contains information about the blocked file/process. Use the
    utility functions defined in `common/include/pmd_ui/utils.h` to
    convert raw data in `BLOCK_OPERATION_DTO` into human-readable formats.

- Strictly follow the rule of Separation of Concerns (SoC) when implementing
    the application. The UI logic should be separated from the data-fetching
    logic. Different distinct modules/classes should be created in
    different classes/files to handle different functionalities.

- Create the files in appropriate directories, following the existing
    project structure. For example, source files regarding UI functionalities
    should be placed in `main/src`, header files in `main/include/pmd_ui/`,
    and so on.

- Do NOT modify any existing files, except those that are empty,
    and those CMakeLists.txt files that need to be updated to include
    new source/header files.

    In case of detecting any existing code that needs to be changed,
    report but do NOT change them.

- Only use Unicode, `wchar_t`, and wide-character Windows API functions.
