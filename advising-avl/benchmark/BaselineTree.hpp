// Generated from original/ProjectTwo.cpp by make_baseline.py.
// Do not edit: comparison counters and height observation only.
class BaselineTree {
private:
    struct Node {
        Course course;
        Node* left;
        Node* right;

        explicit Node(const Course& courseValue)
            : course(courseValue), left(nullptr), right(nullptr) {}
    };

    Node* root;
    inline static std::size_t comparisons_ = 0;

    static Node* insertNode(Node* node, const Course& course, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return new Node(course);
        }

        if (++comparisons_, course.courseNumber < node->course.courseNumber) {
            node->left = insertNode(node->left, course, inserted);
        } else if (++comparisons_, course.courseNumber > node->course.courseNumber) {
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
    BaselineTree() : root(nullptr) {}
    std::size_t comparisons() const { return comparisons_; }
    void resetComparisons() const { comparisons_ = 0; }
    int height() const {
        int height = 0;
        std::vector<std::pair<const Node*,int>> pending;
        if (root) pending.push_back({root,1});
        while (!pending.empty()) {
            auto p = pending.back(); pending.pop_back(); height = std::max(height,p.second);
            if (p.first->left) pending.push_back({p.first->left,p.second+1});
            if (p.first->right) pending.push_back({p.first->right,p.second+1});
        }
        return height;
    }


    ~BaselineTree() {
        clear();
    }

    // Tree ownership is unique; copying raw node pointers would be unsafe.
    BaselineTree(const BaselineTree&) = delete;
    BaselineTree& operator=(const BaselineTree&) = delete;

    bool insert(const Course& course) {
        bool inserted = false;
        root = insertNode(root, course, inserted);
        return inserted;
    }

    const Course* find(const string& courseNumber) const {
        const Node* current = root;

        while (current != nullptr) {
            if (++comparisons_, courseNumber == current->course.courseNumber) {
                return &current->course;
            }

            if (++comparisons_, courseNumber < current->course.courseNumber) {
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
    void swap(BaselineTree& other) {
        std::swap(root, other.root);
    }
};

