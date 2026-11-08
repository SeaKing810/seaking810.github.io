#ifndef ADVISING_AVL_TREE_HPP
#define ADVISING_AVL_TREE_HPP
// Sean McKenna | CS 499 Enhancement Two | November 2026
// AVL balancing preserves sorted traversal while guaranteeing logarithmic
// worst-case insert/find depth. Each node owns its children exclusively.
// Height bookkeeping and rotations trade extra work/storage for predictable
// behavior on ordered input. See docs/ALGORITHM_AND_RESULTS.txt.
#include "Course.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <utility>

class AvlTree {
    struct Node {
        Course course;
        Node* left = nullptr;
        Node* right = nullptr;
        int height = 1;
        explicit Node(const Course& value) : course(value) {}
    };
    Node* root_ = nullptr;
    std::size_t size_ = 0;
    mutable std::size_t comparisons_ = 0;
    std::size_t rotations_ = 0;
    static int heightOf(const Node* n) { return n ? n->height : 0; }
    static void updateHeight(Node* n) {
        n->height = 1 + std::max(heightOf(n->left), heightOf(n->right));
    }
    static int balance(const Node* n) { return heightOf(n->left) - heightOf(n->right); }
    // Rotations preserve in-order keys. Update the demoted node before its parent.
    Node* rotateRight(Node* n) {
        Node* pivot = n->left;
        n->left = pivot->right;
        pivot->right = n;
        updateHeight(n); updateHeight(pivot); ++rotations_;
        return pivot;
    }
    Node* rotateLeft(Node* n) {
        Node* pivot = n->right;
        n->right = pivot->left;
        pivot->left = n;
        updateHeight(n); updateHeight(pivot); ++rotations_;
        return pivot;
    }
    Node* insertNode(Node* n, const Course& course, bool& inserted) {
        if (!n) { Node* leaf = new Node(course); inserted = true; return leaf; }
        ++comparisons_;
        if (course.courseNumber < n->course.courseNumber) {
            n->left = insertNode(n->left, course, inserted);
        } else {
            ++comparisons_;
            if (course.courseNumber > n->course.courseNumber)
                n->right = insertNode(n->right, course, inserted);
            else return n; // Duplicate must not replace the original record.
        }
        if (!inserted) return n;
        updateHeight(n);
        if (balance(n) > 1) {
            if (balance(n->left) < 0) n->left = rotateLeft(n->left); // LR
            return rotateRight(n); // LL, or second half of LR
        }
        if (balance(n) < -1) {
            if (balance(n->right) > 0) n->right = rotateRight(n->right); // RL
            return rotateLeft(n); // RR, or second half of RL
        }
        return n;
    }
    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->left); destroy(n->right); delete n;
    }
    static void appendInOrder(const Node* n, std::vector<Course>& out) {
        if (!n) return;
        appendInOrder(n->left, out); out.push_back(n->course); appendInOrder(n->right, out);
    }
    // Independent verification recomputes heights, key bounds, and node count.
    static int audit(const Node* n, const std::string* low, const std::string* high,
                     bool& ok, std::size_t& count) {
        if (!n) return 0;
        ++count;
        if ((low && n->course.courseNumber <= *low) ||
            (high && n->course.courseNumber >= *high)) ok = false;
        int left = audit(n->left, low, &n->course.courseNumber, ok, count);
        int right = audit(n->right, &n->course.courseNumber, high, ok, count);
        int actual = 1 + std::max(left, right);
        if (n->height != actual || std::abs(left - right) > 1) ok = false;
        return actual;
    }
public:
    AvlTree() = default;
    ~AvlTree() { destroy(root_); }
    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;
    bool insert(const Course& c) {
        bool inserted = false;
        root_ = insertNode(root_, c, inserted);
        if (inserted) ++size_;
        return inserted;
    }
    const Course* find(const std::string& key) const {
        const Node* n = root_;
        while (n) {
            ++comparisons_;
            if (key == n->course.courseNumber) return &n->course;
            ++comparisons_;
            n = key < n->course.courseNumber ? n->left : n->right;
        }
        return nullptr;
    }
    bool empty() const { return !root_; }
    std::size_t size() const { return size_; }
    int height() const { return heightOf(root_); }
    std::size_t comparisons() const { return comparisons_; }
    std::size_t rotations() const { return rotations_; }
    void resetComparisons() const { comparisons_ = 0; }
    bool invariantsHold() const {
        bool ok = true; std::size_t count = 0;
        audit(root_, nullptr, nullptr, ok, count);
        return ok && count == size_;
    }
    std::vector<Course> ordered() const {
        std::vector<Course> out; out.reserve(size_); appendInOrder(root_, out); return out;
    }
    void printCourseList() const {
        for (const auto& c : ordered()) std::cout << c.courseNumber << ", " << c.title << '\n';
    }
    // O(n + p log n) for n records and p prerequisite references, with bounded keys.
    bool validatePrerequisites(std::string& error) const {
        for (const auto& c : ordered()) {
            for (const auto& key : c.prerequisites) {
                if (key == c.courseNumber) {
                    error = "Format error: " + c.courseNumber + " cannot be its own prerequisite.";
                    return false;
                }
                if (!find(key)) {
                    error = "Format error: " + key + " is listed as a prerequisite for " +
                            c.courseNumber + " but is not a course in the file.";
                    return false;
                }
            }
        }
        return true;
    }
    void clear() { destroy(root_); root_ = nullptr; size_ = comparisons_ = rotations_ = 0; }
    // Commit a fully validated candidate in constant time; candidate destroys old state.
    void swap(AvlTree& other) noexcept {
        std::swap(root_, other.root_); std::swap(size_, other.size_);
        std::swap(comparisons_, other.comparisons_); std::swap(rotations_, other.rotations_);
    }
};
#endif
