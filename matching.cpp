#include<bits/stdc++.h>
using namespace std;

#define st first
#define nd second


struct DSU {
    vector<int> link;
    vector<int> directParent;
    void reset(int n) {
        link = vector<int>(n);
        iota(link.begin(), link.end(), 0);
        directParent = vector<int>(n,-1);
    }
    
    int find(int a) {
        return link[a] = (a == link[a] ? a : find(link[a]));
    }

    int operator[](const int& a) {
        return find(a);
    }

    void linkTo(int a, int b) {
        assert(directParent[a] == -1);
        assert(directParent[b] == -1);
        link[find(a)] = find(b);
        directParent[a] = b;
    }

};

enum EdgeType {NotScanned, Prop, Bridge};
struct Edge {
    int from,to;
    int other;
    bool matched;
    EdgeType type = NotScanned;
};

const int INF = 1e9;

int n;
vector<vector<Edge> > graph;
vector<vector<int> > predecessors;
vector<int> removed;
vector<int> evenlvl, oddlvl;
DSU bud;
enum Color {None, Red, Green};
vector<Color> color;
vector<int> parentInDDFSTree;
vector<vector<int> > childsInDDFSTree;

vector<Edge> myBridge;
vector<pair<int,int> > myBudBridge; //bud[bridge]
vector<int> onPathToBottleneck;


int minlvl(int u) { return min(evenlvl[u], oddlvl[u]); }
int maxlvl(int u) { return max(evenlvl[u], oddlvl[u]); }
void setLvl(int u, int i) { if(i&1) oddlvl[u] = i; else evenlvl[u] = i;}

int tenacity(Edge e) {
    if(e.matched)
        return oddlvl[e.from] + oddlvl[e.to] + 1;
    return evenlvl[e.from] + evenlvl[e.to] + 1;
}

/*
tries to move color1 down, updating colors, stacks and parents in ddfs tree
also adds each visited vertex to support of this bridge
terminates either when:
    (*) found reachable vertex in lower layer (possibly after backtracking from this color dfs)
    (*) found bottleneck
    (*) backtracked whole stack and recolored other color's center of activity since no other path not visiting this vertex found
*/
int ddfsMove(vector<int>& stack1, const Color color1, vector<int>& stack2, const Color color2, vector<int>& support) {
    int alternativeParent = -1;
    while(stack1.size() > 0) {
        int u = stack1.back();
        bool foundChild = false;
        for(auto a: predecessors[u]) { //speedup
            int v = bud[a];
            assert(removed[a] == removed[v]);
            if(removed[a])  {
                continue;
            }
            if(color[v] == None) {
                stack1.push_back(v);
                support.push_back(v);
                parentInDDFSTree[v] = u;
                childsInDDFSTree[u].push_back(v);
                color[v] = color1;
                foundChild = true;
                break;
            }
            else if(v == stack2.back()) {
                alternativeParent = u;
            }
        }
        if(!foundChild)
            stack1.pop_back();
        else if(minlvl(stack1.back()) <= minlvl(stack2.back())) //should be ok in both cases: when color1 == Red and when color1 == Green
            return -1;
    }

    if(stack1.size() == 0) {
        if(stack2.size() <= 1) { //can stack2 have one non-bud vertex?
            //found bottleneck
            assert(stack2.size() == 1);
            assert(alternativeParent != -1);
            color[stack2.back()] = None;

            parentInDDFSTree[stack2.back()] = -1; //bottleneck has no parent, but both candidates for parents has it as child 
            childsInDDFSTree[alternativeParent].push_back(stack2.back());
            return stack2.back();
        }
        //change colors
        stack1.push_back(stack2.back());
        assert(color[stack2.back()] == color2);
        color[stack2.back()] = color1;
        assert(alternativeParent != -1);
    
        assert(childsInDDFSTree[parentInDDFSTree[stack2.back()]].back() == stack2.back());
        childsInDDFSTree[parentInDDFSTree[stack2.back()]].pop_back();

        parentInDDFSTree[stack2.back()] = alternativeParent;
        childsInDDFSTree[alternativeParent].push_back(stack2.back());
        stack2.pop_back();
    }
    return -1;
}

//returns {r0, g0} or {bottleneck, bottleneck}
pair<int,int > ddfs(Edge e, vector<int>& out_support) {
    assert(e.type == Bridge);
    vector<int> Sr = {bud[e.from]}, Sg = {bud[e.to]};
    if(Sr[0] == Sg[0]) {
        out_support = {};
        return {Sr[0],Sg[0]};
    }
    out_support = {Sr[0], Sg[0]};
    color[Sr[0]] = Red;
    color[Sg[0]] = Green;

    for(;;) {
        //if found two disjoint paths
        if(minlvl(Sr.back()) == 0 && minlvl(Sg.back()) == 0) {
            out_support.clear();
            return {Sr.back(),Sg.back()};
        }
    
        int b;
        if(minlvl(Sr.back()) >= minlvl(Sg.back()))
            b = ddfsMove(Sr,Red,Sg,Green, out_support);
        else
            b = ddfsMove(Sg,Green,Sr,Red, out_support);
        if(b != -1) {
            out_support.erase(remove(out_support.begin(),out_support.end(),b),out_support.end());
            return {b,b};
        }
    }
    exit(1);
}

bool isVertexMatched(int u) {
    for(auto e:graph[u])
        if(e.matched)
            return true;
    return false;
}

bool openingDfs(int cur, int b,vector<int>& outPath) {
    if(cur == b)
        return true;
    outPath.push_back(cur);
    for(auto a: childsInDDFSTree[cur]) {
        if(!removed[a] && openingDfs(a,b,outPath))
            return true;
    }
    outPath.pop_back();
    return false;
}

void concat(vector<int>& a, const vector<int>& b) {
    for(auto x : b)
        a.push_back(x);
}

vector<int> openEdge(int u, int v);

vector<int> openPath(vector<int> p, bool saveLast = false) {
    vector<int> res;
    for(int i=0;i+1<p.size();i++)
        concat(res,openEdge(p[i],p[i+1]));
    if(saveLast && p.size() > 0)
        res.push_back(p.back());
    return res;
}

vector<int> openEdge2(int u, int v) {
    if(u == v)
        return {};
   if(minlvl(u) == evenlvl[u]) { //simply follow predecessors
        assert(predecessors[u].size() == 1); //u should be evenlevel (last minlevel edge is matched, so there is only one predecessor)
        vector<int> res = {u,predecessors[u][0]};

        int idx = 0;
        while(removed[predecessors[res.back()][idx]]) { 
            idx++; 
            assert(idx < (int)predecessors[res.back()].size());
        }

        concat(res,openEdge2(predecessors[res.back()][idx],v));
        return res;
    }
    else { //through bridge
        int u2 = myBudBridge[u].first, v2 = myBudBridge[u].second;
        int u3 = myBridge[u].from, v3 = myBridge[u].to;
        if((myBudBridge[u2] == myBudBridge[u] && color[u2] != color[u]) || (myBudBridge[v2] == myBudBridge[u] && color[v2] == color[u])) {
            swap(u2, v2);
            swap(u3,v3);
        }
            
        vector<int> p1,p2;
        bool openingDfsSucceed1 = openingDfs(u2,u,p1);
        bool openingDfsSucceed2 = openingDfs(v2,v,p2);

        assert(openingDfsSucceed1);
        assert(openingDfsSucceed2);

        p1.push_back(u);
        p1 = openPath(p1);

        auto p1Prefix = openEdge2(u3,u2);
        concat(p1Prefix, p1);
        p1 = p1Prefix;
        
        p2.push_back(v);
        p2 = openPath(p2);
        auto p2Prefix = openEdge2(v3,v2);
        concat(p2Prefix, p2);
        p2 = p2Prefix;

        reverse(p1.begin(),p1.end());
        vector<int> res = {u};
        concat(p1,p2);
        concat(res,p1);
        return res;
    }
}

vector<int> openEdge(int u, int v) {
    vector<int> res = {u};
    if(find(predecessors[u].begin(),predecessors[u].end(),v) != predecessors[u].end())
        return res;
    for(auto a:predecessors[u]) {
        int cur = bud.directParent[a];
        while(cur != -1) { //speedup
            if(cur == v) {
                concat(res,openEdge2(a,v));
                return res;
            }
            cur = bud.directParent[cur];
        }
    }
    cerr<<"sth went wrong in openEdge..."<<endl;
    exit(1);
}

bool checkIfIsGoodAlternatingPath(vector<int> x) {
    bool lastMatched = false;
    for(int i=0;i+1<x.size();i++) {
        bool found = false;
        for(auto e:graph[x[i]]) {
            if(e.to == x[i+1]) {
                if(i > 0 && e.matched == lastMatched) {
                    cerr<<"not alternating"<<endl;
                    return false;
                }
                lastMatched = e.matched;
                found = true;
                break;
            }
        }
        if(!found) {
            cerr<<"invalid path"<<endl;
            return false;
        }
    }
    return true;
}

bool checkIfIsGoodAugumentingPath(vector<int> x) {
    return checkIfIsGoodAlternatingPath(x) && x.size()%2 == 0 && !isVertexMatched(x[0]);

}

bool bfs() {
    vector<vector<int> > verticesAtLevel(n);
    vector<vector<Edge> > bridges(2*n+2);

    removed.clear();
    removed.resize(n);
    vector<int> removedPredecessorsSize(n);

    for(int u=0;u<n;u++)
        if(!isVertexMatched(u)) {
            verticesAtLevel[0].push_back(u);
            setLvl(u,0);
        }

    bool foundPath = false;  
    for(int i=0;i<n && !foundPath;i++) {
        for(auto u : verticesAtLevel[i]) {
            for(auto& e:graph[u]) {
                if(e.type == NotScanned && ((oddlvl[u] == i && e.matched) || (evenlvl[u] == i && !e.matched))) { //fix
                    if(minlvl(e.to) >= i+1) {
                        e.type = Prop;
                        graph[e.to][e.other].type = Prop;

                        if(minlvl(e.to) > i+1) {
                            setLvl(e.to,i+1);
                            verticesAtLevel[i+1].push_back(e.to);
                        }
                        predecessors[e.to].push_back(u);
                    }
                    else {
                        e.type = Bridge;
                        graph[e.to][e.other].type = Bridge;
                        if(tenacity(e) < INF) {
                            bridges[tenacity(e)].push_back(e);
                        }
                    }
                }
            }
        }
        

        for(auto b : bridges[2*i+1]) {
            if(removed[b.from] || removed[b.to])
                continue;
            vector<int> support;
            pair<int,int> ddfsResult = ddfs(b,support);
            if(ddfsResult.first == ddfsResult.second) {
                pair<int,int> budBridge = {bud[b.from], bud[b.to]};
                for(auto v:support) {
                    setLvl(v,2*i+1-minlvl(v));
                    verticesAtLevel[2*i+1-minlvl(v)].push_back(v);
                    myBridge[v] = b;
                    myBudBridge[v] = budBridge;
                    bud.linkTo(v,ddfsResult.first);

                    if(evenlvl[v] > oddlvl[v]) {
                        for(auto f : graph[v]) {
                            if(f.type == Bridge && tenacity(f) < INF && !f.matched) {
                                bridges[tenacity(f)].push_back(f);
                            }
                        }
                    }
                }
            }
            else {
                if(color[ddfsResult.first] != color[bud[b.from]])
                    swap(ddfsResult.first, ddfsResult.second);

                vector<int> p,q;
                for(auto v : {ddfsResult.first, ddfsResult.second}) {
                    p.push_back(v);
                    while(p.back() != bud[b.to] && p.back() != bud[b.from])
                        p.push_back(parentInDDFSTree[p.back()]);
                    swap(p,q);
                }
                reverse(p.begin(),p.end());
                reverse(q.begin(),q.end());
                p = openPath(p,true);
                q = openPath(q,true);
                auto x = openEdge2(b.from, bud[b.from]);
                auto y = openEdge2(b.to, bud[b.to]);
                
                concat(x,p);
                concat(y,q);
                reverse(x.begin(),x.end());
                concat(x,y);
                if(!checkIfIsGoodAugumentingPath(x)) {
                    cerr<<"wrong augumenting path!"<<endl;
                    exit(1);
                }
                foundPath = true;
                queue<int> removingVerticesQueue;
                for(auto &v: x) {
                    removed[v] = true;
                    removingVerticesQueue.push(v);
                }

                for(int j=0;j+1<x.size();j++) {
                    for(auto &e: graph[x[j]]) {
                        if(e.to == x[j+1]) {
                            e.matched ^= 1;
                            graph[e.to][e.other].matched ^= 1;
                        }
                    }
                }
                while(!removingVerticesQueue.empty()) {
                    int v = removingVerticesQueue.front();
                    removingVerticesQueue.pop();
                    for(auto e : graph[v]) {
                        if(e.type == Prop && minlvl(e.to) > minlvl(e.from) && !removed[e.to]) {
                            removedPredecessorsSize[e.to]++;
                            if(removedPredecessorsSize[e.to] == predecessors[e.to].size()) {
                                removed[e.to] = true;
                                removingVerticesQueue.push(e.to);
                            }
                        }
                    }
                }
            }
        }
    }
    return foundPath;
}

int32_t main(){
    ios::sync_with_stdio(false);
    int m;
    cin >> n >> m;
    graph.resize(n);
    for(int i=0;i<m;i++) {
        int a,b,isMatched;
        cin >> a >> b >> isMatched;
        //isMatched = false;
        graph[a].push_back({a, b, (int)graph[b].size(), isMatched, NotScanned});
        graph[b].push_back({b, a, (int)graph[a].size()-1, isMatched, NotScanned});
    }
    int iters = 0;
    do {
        iters++;
        for(auto&a: graph) {
            for(auto&e:a)
                e.type = NotScanned;
        }
        predecessors.clear();
        predecessors.resize(n);
        evenlvl.clear();
        evenlvl.resize(n,INF);
        oddlvl.clear();
        oddlvl.resize(n,INF);


        bud.reset(n);
        color.clear();
        color.resize(n,None);
        parentInDDFSTree.clear();
        parentInDDFSTree.resize(n,-1);
        childsInDDFSTree.clear();
        childsInDDFSTree.resize(n);
        myBudBridge.clear();
        myBudBridge.resize(n);
        myBridge.clear();
        myBridge.resize(n,{-1,-1,-1,false,NotScanned});
    }while(bfs());

    int cnt = 0;
    for(int i=0;i<n;i++)
        if(isVertexMatched(i))
            cnt++;
    cout<<cnt/2<<endl;//<<" after "<<iters<<" iterations"<<endl;
}