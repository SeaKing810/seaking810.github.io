//============================================================================
// Name        : ProjectTwo.cpp
// Description : ABCU advising assistance program
//============================================================================

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// One Course object stores the data from one nonblank CSV record.
struct Course {
    string courseNumber;
    string title;
    vector<string> prerequisites;
};

// Stores Course objects by course number. An in-order traversal produces the
// course list in ascending alphanumeric order.
class BinarySearchTree {
private:
    struct Node {
        Course course;
        Node* left;
        Node* right;

        explicit Node(const Course& courseValue)
            : course(courseValue), left(nullptr), right(nullptr) {}
    };

    Node* root;

    static Node* insertNode(Node* node, const Course& course, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return new Node(course);
        }

        if (course.courseNumber < node->course.courseNumber) {
            node->left = insertNode(node->left, course, inserted);
        } else if (course.courseNumber > node->course.courseNumber) {
            node->right = insertNode(node->right, course, inserted);
        } else {
            inserted = false;
        }

        return node;
    }

    static void printInOrder(const Node* node) {
        if (node == nullptr) {
            return;
        }

        printInOrder(node->left);
        cout << node->course.courseNumber << ", " << node->course.title << '\n';
        printInOrder(node->right);
    }

    static void deleteTree(Node* node) {
        if (node == nullptr) {
            return;
        }

        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

    bool validateNode(const Node* node, string& errorMessage) const {
        if (node == nullptr) {
            return true;
        }

        if (!validateNode(node->left, errorMessage)) {
            return false;
        }

        for (const string& prerequisiteNumber : node->course.prerequisites) {
            if (prerequisiteNumber == node->course.courseNumber) {
                errorMessage = "Format error: " + node->course.courseNumber
                    + " cannot be its own prerequisite.";
                return false;
            }

            if (find(prerequisiteNumber) == nullptr) {
                errorMessage = "Format error: " + prerequisiteNumber
                    + " is listed as a prerequisite for "
                    + node->course.courseNumber
                    + " but is not a course in the file.";
                return false;
            }
        }

        return validateNode(node->right, errorMessage);
    }

public:
    BinarySearchTree() : root(nullptr) {}

    ~BinarySearchTree() {
        clear();
    }

    // Tree ownership is unique; copying raw node pointers would be unsafe.
    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    bool insert(const Course& course) {
        bool inserted = false;
        root = insertNode(root, course, inserted);
        return inserted;
    }

    const Course* find(const string& courseNumber) const {
        const Node* current = root;

        while (current != nullptr) {
            if (courseNumber == current->course.courseNumber) {
                return &current->course;
            }

            if (courseNumber < current->course.courseNumber) {
                current = current->left;
            } else {
                current = current->right;
            }
        }

        return nullptr;
    }

    bool empty() const {
        return root == nullptr;
    }

    void printCourseList() const {
        printInOrder(root);
    }

    bool validatePrerequisites(string& errorMessage) const {
        return validateNode(root, errorMessage);
    }

    void clear() {
        deleteTree(root);
        root = nullptr;
    }

    // Swapping lets a fully validated temporary tree replace the active tree.
    void swap(BinarySearchTree& other) {
        std::swap(root, other.root);
    }
};

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

// Loads into a temporary tree first. Existing valid data is replaced only when
// the complete new file passes both parsing and prerequisite validation.
bool loadCourses(const string& fileName, BinarySearchTree& courses) {
    ifstream inputFile(fileName.c_str());

    if (!inputFile.is_open()) {
        cout << "Error: unable to open the course data file.\n";
        return false;
    }

    BinarySearchTree parsedCourses;
    string line;
    size_t lineNumber = 0;

    while (getline(inputFile, line)) {
        ++lineNumber;

        // Blank physical lines do not represent course records.
        if (trim(line).empty()) {
            continue;
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

        Course course;
        course.courseNumber = normalizeCourseNumber(fields[0]);
        course.title = fields[1];

        // The supplied file uses empty trailing columns for courses with fewer
        // prerequisites. Ignore those trailing blanks while rejecting gaps.
        size_t prerequisiteEnd = fields.size();
        while (prerequisiteEnd > 2 && fields[prerequisiteEnd - 1].empty()) {
            --prerequisiteEnd;
        }

        for (size_t index = 2; index < prerequisiteEnd; ++index) {
            const string prerequisiteNumber = normalizeCourseNumber(fields[index]);

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

    courses.swap(parsedCourses);
    cout << "Course data loaded successfully.\n";
    return true;
}

void displayCourseList(const BinarySearchTree& courses) {
    cout << "Here is a sample schedule:\n\n";
    courses.printCourseList();
}

void displayCourseInformation(const BinarySearchTree& courses,
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

int main() {
    BinarySearchTree courses;
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
