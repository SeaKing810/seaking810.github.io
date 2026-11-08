"""Derive instrumentation from the unchanged original, keeping its tree algorithm.
Only comparison counters and iterative height inspection are added. No balancing.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'original/ProjectTwo.cpp').read_text()
s=s[s.index('class BinarySearchTree'):s.index('// Removes whitespace')]
s=s.replace('BinarySearchTree','BaselineTree')
s=s.replace('    Node* root;','    Node* root;\n    inline static std::size_t comparisons_ = 0;')
for expr in ['course.courseNumber < node->course.courseNumber','course.courseNumber > node->course.courseNumber','courseNumber == current->course.courseNumber','courseNumber < current->course.courseNumber']:
 s=s.replace('('+expr+')','(++comparisons_, '+expr+')')
marker='    BaselineTree() : root(nullptr) {}'
s=s.replace(marker,marker+'''
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
''')
(root/'benchmark/BaselineTree.hpp').write_text('// Generated from original/ProjectTwo.cpp by make_baseline.py.\n// Do not edit: comparison counters and height observation only.\n'+s)
