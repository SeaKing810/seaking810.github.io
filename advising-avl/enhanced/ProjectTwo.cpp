// Sean McKenna | CS 499 | November 2026
// Enhanced CS 300 workflow; AVL index plus bounded, all-or-nothing file loading.
#include "AvlTree.hpp"
#include <fstream>
#include <cctype>
#include <stdexcept>
using namespace std;

// Removes whitespace that can appear around user input and CSV fields.
string trim(const string& text) {
    const string whitespace = " \t\n\r\f\v";
    const size_t first = text.find_first_not_of(whitespace);

    if (first == string::npos) {
        return "";
    }

    const size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

string toUpper(string text) {
    transform(text.begin(), text.end(), text.begin(),
        [](unsigned char character) {
            return static_cast<char>(toupper(character));
        });
    return text;
}

string normalizeCourseNumber(const string& courseNumber) {
    return toUpper(trim(courseNumber));
}

// Splits a simple comma-separated record and preserves empty fields. The
// assignment data does not contain quoted commas, so no external CSV parser is
// needed.
vector<string> splitCsvLine(const string& line) {
    vector<string> fields;
    size_t start = 0;

    while (true) {
        const size_t comma = line.find(',', start);

        if (comma == string::npos) {
            fields.push_back(line.substr(start));
            break;
        }

        fields.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }

    return fields;
}

string cleanFileName(const string& input) {
    string fileName = trim(input);

    // Accept paths pasted with matching single or double quotation marks.
    if (fileName.size() >= 2) {
        const bool doubleQuoted = fileName.front() == '"' && fileName.back() == '"';
        const bool singleQuoted = fileName.front() == '\'' && fileName.back() == '\'';

        if (doubleQuoted || singleQuoted) {
            fileName = trim(fileName.substr(1, fileName.size() - 2));
        }
    }

    return fileName;
}

// File limits apply before accumulating an unbounded line or candidate tree.
namespace Limits {
constexpr size_t fileBytes = 2 * 1024 * 1024;
constexpr size_t lineBytes = 4096;
constexpr size_t records = 10000;
constexpr size_t fieldBytes = 256;
constexpr size_t keyBytes = 32;
constexpr size_t prerequisites = 32;
}
// Count actual bytes read (including line endings). No full-file buffering.
bool boundedLine(istream& in, string& line, size_t& bytes) {
    line.clear(); char c;
    while (in.get(c)) {
        if (++bytes > Limits::fileBytes) throw runtime_error("File exceeds 2 MiB limit.");
        if (c == '\n') return true;
        if (line.size() >= Limits::lineBytes) throw runtime_error("Line exceeds 4096 bytes.");
        if (c == '\0') throw runtime_error("NUL bytes are not allowed.");
        line.push_back(c);
    }
    if (in.bad() || (in.fail() && !in.eof())) throw runtime_error("Course data could not be read.");
    return !line.empty();
}

// Loads into a temporary tree first. Existing valid data is replaced only when
// the complete new file passes both parsing and prerequisite validation.
bool loadCoursesFromStream(istream& inputFile, AvlTree& courses) {
    AvlTree parsedCourses;
    string line;
    size_t lineNumber = 0;

    size_t bytes = 0;
    try {
        while (boundedLine(inputFile, line, bytes)) {
            ++lineNumber;

            // Blank physical lines do not represent course records.
            if (trim(line).empty()) {
                continue;
            }

            if (parsedCourses.size() >= Limits::records) {
                cout << "Format error: at most 10000 courses are allowed.\n"; return false;
            }
            if (line.find('"') != string::npos) {
                cout << "Format error: quoted CSV fields are not supported.\n"; return false;
            }
            vector<string> fields = splitCsvLine(line);
            for (string& field : fields) {
                field = trim(field);
            }

            // Remove a UTF-8 byte-order mark if one appears at the file start.
            if (lineNumber == 1 && !fields.empty()
                    && fields[0].compare(0, 3, "\xEF\xBB\xBF") == 0) {
                fields[0].erase(0, 3);
            }

            if (fields.size() < 2 || fields[0].empty() || fields[1].empty()) {
                cout << "Format error on line " << lineNumber
                     << ": a course number and title are required.\n";
                return false;
            }

            for (const auto& field : fields) {
                if (field.size() > Limits::fieldBytes) {
                    cout << "Format error: field exceeds 256 bytes.\n"; return false;
                }
            }
            if (fields[0].size() > Limits::keyBytes) {
                cout << "Format error: course key exceeds 32 bytes.\n"; return false;
            }
            Course course;
            course.courseNumber = normalizeCourseNumber(fields[0]);
            course.title = fields[1];

            // The supplied file uses empty trailing columns for courses with fewer
            // prerequisites. Ignore those trailing blanks while rejecting gaps.
            size_t prerequisiteEnd = fields.size();
            while (prerequisiteEnd > 2 && fields[prerequisiteEnd - 1].empty()) {
                --prerequisiteEnd;
            }

            if (prerequisiteEnd - 2 > Limits::prerequisites) {
                cout << "Format error: at most 32 prerequisites are allowed.\n"; return false;
            }
            for (size_t index = 2; index < prerequisiteEnd; ++index) {
                const string prerequisiteNumber = normalizeCourseNumber(fields[index]);

                if (prerequisiteNumber.size() > Limits::keyBytes) {
                    cout << "Format error: prerequisite key exceeds 32 bytes.\n"; return false;
                }
                if (prerequisiteNumber.empty()) {
                    cout << "Format error on line " << lineNumber
                         << ": a prerequisite field is empty.\n";
                    return false;
                }

                if (find(course.prerequisites.begin(), course.prerequisites.end(),
                        prerequisiteNumber) != course.prerequisites.end()) {
                    cout << "Format error on line " << lineNumber
                         << ": duplicate prerequisite " << prerequisiteNumber << ".\n";
                    return false;
                }

                course.prerequisites.push_back(prerequisiteNumber);
            }

            if (!parsedCourses.insert(course)) {
                cout << "Format error on line " << lineNumber
                     << ": duplicate course " << course.courseNumber << ".\n";
                return false;
            }
        }

        if (inputFile.bad()) {
            cout << "Error: the course data file could not be read.\n";
            return false;
        }

        if (parsedCourses.empty()) {
            cout << "Format error: the course data file contains no courses.\n";
            return false;
        }

        // References are checked only after every course number has been loaded.
        string validationError;
        if (!parsedCourses.validatePrerequisites(validationError)) {
            cout << validationError << '\n';
            return false;
        }

    } catch (const exception& error) {
        cout << "Error: " << error.what() << " Active catalog unchanged.\n";
        return false;
    }

    courses.swap(parsedCourses);
    cout << "Course data loaded successfully.\n";
    return true;
}

bool loadCourses(const string& fileName, AvlTree& courses) {
    ifstream inputFile(fileName, ios::binary);
    if (!inputFile) { cout << "Error: unable to open the course data file.\n"; return false; }
    return loadCoursesFromStream(inputFile, courses);
}

void displayCourseList(const AvlTree& courses) {
    cout << "Here is a sample schedule:\n\n";
    courses.printCourseList();
}

void displayCourseInformation(const AvlTree& courses,
        const string& courseNumberInput) {
    const string courseNumber = normalizeCourseNumber(courseNumberInput);

    if (courseNumber.empty()) {
        cout << "A course number is required.\n";
        return;
    }

    const Course* course = courses.find(courseNumber);
    if (course == nullptr) {
        cout << "Course " << courseNumber << " was not found.\n";
        return;
    }

    cout << course->courseNumber << ", " << course->title << '\n';

    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None\n";
        return;
    }

    cout << "Prerequisites:\n";
    for (const string& prerequisiteNumber : course->prerequisites) {
        const Course* prerequisite = courses.find(prerequisiteNumber);

        // All references were validated during loading, so this is expected to
        // succeed for every prerequisite in a loaded tree.
        if (prerequisite != nullptr) {
            cout << prerequisite->courseNumber << ", "
                 << prerequisite->title << '\n';
        }
    }
}

void displayMenu() {
    cout << "\n"
         << "    1. Load Data Structure.\n"
         << "    2. Print Course List.\n"
         << "    3. Print Course.\n"
         << "    9. Exit\n\n";
}

#ifndef ADVISING_NO_MAIN
int main() {
    AvlTree courses;
    string menuInput;

    cout << "Welcome to the course planner.\n";

    while (true) {
        displayMenu();
        cout << "What would you like to do? ";

        if (!getline(cin, menuInput)) {
            cout << "\nThank you for using the course planner!\n";
            return 0;
        }

        const string choice = trim(menuInput);

        if (choice == "1") {
            string fileNameInput;
            cout << "Enter the course data file name: ";

            if (!getline(cin, fileNameInput)) {
                cout << "\nThank you for using the course planner!\n";
                return 0;
            }

            const string fileName = cleanFileName(fileNameInput);
            if (fileName.empty()) {
                cout << "Error: a file name is required.\n";
            } else {
                loadCourses(fileName, courses);
            }
        } else if (choice == "2") {
            if (courses.empty()) {
                cout << "Please load course data before printing the course list.\n";
            } else {
                displayCourseList(courses);
            }
        } else if (choice == "3") {
            if (courses.empty()) {
                cout << "Please load course data before looking up a course.\n";
            } else {
                string courseNumber;
                cout << "What course do you want to know about? ";

                if (!getline(cin, courseNumber)) {
                    cout << "\nThank you for using the course planner!\n";
                    return 0;
                }

                displayCourseInformation(courses, courseNumber);
            }
        } else if (choice == "9") {
            cout << "Thank you for using the course planner!\n";
            return 0;
        } else if (choice.empty()) {
            cout << "Please enter 1, 2, 3, or 9.\n";
        } else {
            cout << choice << " is not a valid option.\n";
        }
    }
}

#endif
