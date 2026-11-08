#ifndef ADVISING_COURSE_HPP
#define ADVISING_COURSE_HPP
#include <string>
#include <vector>
// Same record model as the original CS 300 artifact.
struct Course {
    std::string courseNumber;
    std::string title;
    std::vector<std::string> prerequisites;
};
#endif
