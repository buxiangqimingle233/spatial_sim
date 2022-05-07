#ifndef __PATH_HPP__
#define __PATH_HPP__

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <queue>

using namespace std;

typedef pair<int, int> Segment;
const int TREESTART = -2;
const int INVALID = -3;

// The tree-based multicast path. It enables individual non-optimal
// routing for each edge within the tree. 
class MCTree {
public:
    map<Segment, shared_ptr<queue<int>> > _path;   // detour: [src, dst]=intermediate_nodes
    multimap<int, int> _tree;                       // branch tree: start-end
    set<int> _leafs;                                // destination nodes

public:

    int root() {
        assert(_tree.count(TREESTART) == 1);
        return _tree.find(TREESTART)->second;
    }

    vector<Segment> segmentStartedWith(int start) {
        vector<Segment> ret;
        auto range = _tree.equal_range(start);
        for (auto iter = range.first; iter != range.second; ++iter) {
            ret.push_back(Segment(make_pair(iter->first, iter->second)));
        }
        return ret;
    }

    shared_ptr<queue<int>> intermediateNodes(Segment seg) {
        assert(_path.count(seg) != 0);
        return _path[seg];
    }

    void setDestNodes(const set<int>& dests) {
        _leafs = dests;
    }

    set<int> getDestNodes() {
        return _leafs;
    }

    bool isDestNode(int nid) {
        return _leafs.count(nid);
    }

    void addSegment(int src, int dst, shared_ptr<queue<int>> im_nodes, bool eject = false) {
        assert(src != dst);
        // insert intermediate nodes
        if (im_nodes == nullptr) {
            _path.insert(make_pair(make_pair(src, dst), make_shared<queue<int>>()));
        } else {
            _path.insert(make_pair(make_pair(src, dst), im_nodes));
        }
        // insert destination nodes
        if (eject) {
            _leafs.insert(dst);
        }
        // build tree
        _tree.insert(make_pair(src, dst));
        assert(_tree.count(src) <= 4);
    }

    void buildTree() {
        if (_tree.count(TREESTART) == 0) {
            std::cerr << "Path construction error: " << " the root is missing" << std::endl;
            assert(false);
        }

        for (auto iter = _path.begin(); iter != _path.end(); ++iter) {
            const int seg_start = iter->first.first, seg_end = iter->first.second;
            if (_tree.count(seg_start) == 0 && seg_start != TREESTART) {
                std::cerr << "Path construction error: " << "Segment " << seg_start << "-" << seg_end \
                        << " has no ancestors." << std::endl;
                assert(false);
            }
            if (_tree.count(seg_end) == 0 && _leafs.count(seg_end) == 0) {
                std::cerr << "Path construction error: " << "Segment " << seg_start << "-" << seg_end \
                        << " has no succeeds, but it doesn't point to any destination." << std::endl;
                assert(false);
            }
        }
    }

    MCTree(int root) { 
        _tree.insert(make_pair(TREESTART, root));
        _path.insert(make_pair(make_pair(TREESTART, root), make_shared<queue<int>>()));
    }

};


#endif