// Behavioral, property, and input-boundary tests for the complete enhancement.
#define ADVISING_NO_MAIN
#include "../enhanced/ProjectTwo.cpp"
#include <functional>
#include <map>
#include <numeric>
#include <random>
#include <sstream>

void require(bool value, const string& why) { if (!value) throw runtime_error(why); }
Course course(string key) { return {key, "Title " + key, {}}; }
string key(int n) { string s = to_string(n); return "C" + string(5 - s.size(), '0') + s; }
struct Capture {
    ostringstream output; streambuf* old;
    Capture() : old(cout.rdbuf(output.rdbuf())) {}
    ~Capture() { cout.rdbuf(old); }
};
bool load(const string& text, AvlTree& tree) {
    istringstream in(text); Capture quiet; return loadCoursesFromStream(in, tree);
}
void rejected(const string& text) {
    AvlTree tree; require(load("KEEP,Retained\n", tree), "setup");
    require(!load(text, tree), "invalid file accepted");
    require(tree.size() == 1 && tree.find("KEEP") && tree.invariantsHold(), "active state changed");
}
int main() {
    size_t passed = 0;
    auto test = [&](const string& name, const function<void()>& action) {
        try { action(); ++passed; cout << "PASS " << name << '\n'; }
        catch (const exception& e) { cerr << "FAIL " << name << ": " << e.what() << '\n'; exit(1); }
    };
    test("empty and single-node behavior", [] {
        AvlTree t; require(t.empty() && t.height() == 0 && !t.find("X") && t.invariantsHold(), "empty");
        require(t.insert(course("B")) && t.height() == 1 && t.find("B"), "single");
    });
    for (const auto& sequence : vector<vector<string>>{{"C","B","A"},{"A","B","C"},{"C","A","B"},{"A","C","B"}}) {
        test("rotation " + sequence[0] + sequence[1] + sequence[2], [=] {
            AvlTree t;
            for (const auto& k : sequence) { require(t.insert(course(k)), "insert"); require(t.invariantsHold(), "invariant"); }
            require(t.height() == 2, "height");
            require(t.rotations() == (sequence[1] == "B" ? 1U : 2U), "rotation count");
            require(t.ordered()[0].courseNumber == "A" && t.ordered()[2].courseNumber == "C", "order");
        });
    }
    test("duplicate preserves complete original record", [] {
        AvlTree t; t.insert({"A","Original",{"B"}});
        require(!t.insert({"A","Replacement",{}}), "accepted duplicate");
        require(t.size() == 1 && t.find("A")->title == "Original" && t.find("A")->prerequisites == vector<string>{"B"}, "mutated");
    });
    for (int mode = 0; mode < 3; ++mode) test("property sequence " + to_string(mode), [=] {
        vector<int> values(2000); iota(values.begin(), values.end(), 0);
        if (mode == 1) reverse(values.begin(), values.end());
        if (mode == 2) { mt19937 rng(499); shuffle(values.begin(), values.end(), rng); }
        AvlTree t; map<string, Course> expected;
        for (int n : values) {
            auto c = course(key(n)); expected.emplace(c.courseNumber, c);
            require(t.insert(c) && t.invariantsHold(), "insert invariant");
            auto ordered = t.ordered(); require(ordered.size() == expected.size(), "size");
            size_t i = 0;
            for (const auto& entry : expected) require(ordered[i++].courseNumber == entry.first, "ordering differs from map");
        }
        for (int n : values) require(t.find(key(n))->title == "Title " + key(n), "lookup mismatch");
        require(!t.find("ABSENT") && t.height() <= 16, "height or missing lookup");
    });
    test("clear and swap ownership", [] {
        AvlTree a,b; a.insert(course("A")); b.insert(course("B")); b.insert(course("C"));
        a.swap(b); require(a.size()==2 && b.size()==1 && a.find("B") && b.find("A"), "swap");
        b.clear(); b.clear(); require(b.empty() && b.invariantsHold() && a.invariantsHold(), "clear");
    });
    test("original course fixture", [] {
        AvlTree t; Capture quiet; require(loadCourses("original/ABCU_Advising_Program_Input.csv",t), "load");
        require(t.size()==8 && t.find("CSCI400")->prerequisites.size()==2 && t.invariantsHold(), "records");
    });
    test("normalization BOM CRLF trailing blanks and forward references", [] {
        AvlTree t;
        require(load("\xEF\xBB\xBF a , Alpha , b ,\r\n\r\n b,Beta,,\r\n",t), "format");
        require(t.find("A")->prerequisites == vector<string>{"B"}, "normalize");
    });
    test("valid replacement removes prior records", [] {
        AvlTree t; require(load("A,Alpha\n",t) && load("B,Beta",t), "load");
        require(t.size()==1 && !t.find("A") && t.find("B"), "replace");
    });
    vector<pair<string,string>> invalid = {
        {"empty file",""}, {"blank file"," \n\n"}, {"missing title","A,\n"},
        {"missing course"," ,Alpha\n"}, {"missing comma","A\n"},
        {"duplicate normalized keys","a,Alpha\n A ,Other\n"},
        {"missing prerequisite","A,Alpha,B\n"}, {"self prerequisite","A,Alpha,a\n"},
        {"duplicate prerequisite","A,Alpha,B,b\nB,Beta\n"},
        {"interior prerequisite gap","A,Alpha,,B\nB,Beta\n"},
        {"quoted fields","A,\"Alpha, Beta\"\n"},
        {"NUL byte",string("A,A\0lpha\n",10)},
        {"overlong key",string(33,'A')+",Title\n"},
        {"overlong title","A,"+string(257,'T')+"\n"},
        {"overlong prerequisite","A,Title,"+string(33,'B')+"\n"},
        {"overlong line",string(4097,' ') + "\n"}
    };
    for (const auto& item : invalid) test(item.first + " preserves prior catalog", [=] { rejected(item.second); });
    test("maximum key and title lengths accepted", [] {
        AvlTree t; require(load(string(32,'A')+","+string(256,'T')+"\n",t), "boundary");
    });
    test("maximum line length accepted", [] { AvlTree t; require(load("A,T"+string(4093,' ')+"\n",t), "line boundary"); });
    test("file byte boundary and limit", [] {
        AvlTree t; string exact = string(Limits::fileBytes-4,'\n')+"A,T\n";
        require(load(exact,t), "exact byte limit"); rejected(exact+"\n");
    });
    test("record boundary and limit", [] {
        string data; for(int i=0;i<10000;++i) data += key(i)+",Title\n";
        AvlTree t; require(load(data,t) && t.size()==10000 && t.invariantsHold(), "record boundary");
        rejected(data+"EXTRA,Title\n");
    });
    test("prerequisite boundary and limit", [] {
        string data="A,Title";
        for(int i=0;i<32;++i) data+=","+key(i);
        string tail="\n"; for(int i=0;i<33;++i) tail+=key(i)+",Title\n";
        AvlTree t; require(load(data+tail,t), "32 prerequisites"); rejected(data+","+key(32)+tail);
    });
    test("missing file preserves catalog", [] {
        AvlTree t; t.insert(course("KEEP")); Capture quiet;
        require(!loadCourses("does-not-exist.csv",t) && t.find("KEEP"), "missing file");
    });
    test("I/O failure preserves catalog", [] {
        AvlTree t; t.insert(course("KEEP")); istringstream in("A,Alpha"); in.setstate(ios::badbit);
        Capture quiet; require(!loadCoursesFromStream(in,t) && t.find("KEEP"), "bad stream");
    });
    test("indirect cycles retain original policy", [] {
        AvlTree t; require(load("A,Alpha,B\nB,Beta,A\n",t), "baseline allows indirect cycles");
    });
    cout << "RESULT: " << passed << " test groups passed.\n";
}
