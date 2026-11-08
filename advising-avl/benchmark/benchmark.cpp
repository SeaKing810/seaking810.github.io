// Compare original BST logic with AVL under identical synthetic workloads.
#include "../enhanced/AvlTree.hpp"
#include <chrono>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
using namespace std;
#include "BaselineTree.hpp"
using Clock = chrono::steady_clock;
volatile size_t consumed = 0;
struct Result { double buildUs, findUs; size_t insertComparisons, findComparisons; int height; };
string key(int i) { string s=to_string(i); return "C"+string(5-s.size(),'0')+s; }
template<class Tree> Result measure(const vector<Course>& input, const vector<string>& queries,
                                  const string& expectedList) {
    Tree tree; tree.resetComparisons();
    auto start=Clock::now();
    for (const auto& c:input) if(!tree.insert(c)) throw runtime_error("duplicate");
    auto end=Clock::now();
    double build=chrono::duration<double,micro>(end-start).count();
    size_t inserted=tree.comparisons(); int height=tree.height();
    tree.resetComparisons(); size_t checksum=0;
    start=Clock::now();
    for (const auto& q:queries) { const Course* c=tree.find(q); if(c) checksum+=c->title.size(); }
    end=Clock::now();
    double find=chrono::duration<double,micro>(end-start).count();
    size_t searched=tree.comparisons(); consumed+=checksum;
    // Data correctness, traversal, and destruction remain outside timed regions.
    size_t expectedChecksum=0;
    for (const auto& c:input) {
        const Course* found=tree.find(c.courseNumber);
        if(!found || found->title!=c.title || found->prerequisites!=c.prerequisites)
            throw runtime_error("record mismatch");
        expectedChecksum+=c.title.size();
    }
    if(checksum!=expectedChecksum) throw runtime_error("lookup mismatch");
    ostringstream output; auto old=cout.rdbuf(output.rdbuf()); tree.printCourseList(); cout.rdbuf(old);
    if(output.str()!=expectedList) throw runtime_error("ordered output mismatch");
    return {build,find,inserted,searched,height};
}
int main() {
    cout << "n,order,tree,trial,build_us,lookup_us,insert_comparisons,lookup_comparisons,height\n";
    for(int n:{100,1000,5000}) {
        vector<Course> canonical;
        string expected;
        for(int i=0;i<n;++i) { canonical.push_back({key(i),"Course "+key(i),{}}); expected+=key(i)+", Course "+key(i)+"\n"; }
        vector<string> queries; for(int i=0;i<n;++i) queries.push_back(key(i));
        for(int i=0;i<n;++i) queries.push_back("Z"+key(i)); // n misses above max key
        mt19937 qrng(499); shuffle(queries.begin(),queries.end(),qrng);
        for(string order:{"sorted","reverse","shuffled"}) {
            auto data=canonical;
            if(order=="reverse") reverse(data.begin(),data.end());
            if(order=="shuffled") { mt19937 rng(499); shuffle(data.begin(),data.end(),rng); }
            // Trial 0 warms both implementations; five measured trials alternate order.
            for(int trial=0;trial<=5;++trial) {
                Result bst{},avl{};
                if(trial%2==0) { bst=measure<BaselineTree>(data,queries,expected); avl=measure<AvlTree>(data,queries,expected); }
                else { avl=measure<AvlTree>(data,queries,expected); bst=measure<BaselineTree>(data,queries,expected); }
                if(!trial) continue;
                for(auto result:{make_pair("BST",bst),make_pair("AVL",avl)}) {
                    auto r=result.second;
                    cout<<n<<','<<order<<','<<result.first<<','<<trial<<','<<fixed<<setprecision(3)
                        <<r.buildUs<<','<<r.findUs<<','<<r.insertComparisons<<','<<r.findComparisons<<','<<r.height<<'\n';
                }
            }
        }
    }
}
