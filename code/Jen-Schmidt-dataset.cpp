#include <bits/stdc++.h>
#include <chrono>
using namespace std;

// Iterative DFS to build DFS tree and numbering (avoids stack overflow)
void dfs(vector<vector<int>> &g, vector<vector<int>> &dfs_tree, int start, int w_start, vector<int> &visited, vector<int> &numbering)
{
    struct Frame { int node; int idx; };
    stack<Frame> stk;
    visited[start] = 1;
    numbering[w_start] = start;
    int w = w_start + 1;
    stk.push({start, 0});
    while (!stk.empty())
    {
        Frame &f = stk.top();
        if (f.idx < (int)g[f.node].size())
        {
            int v = g[f.node][f.idx];
            f.idx++;
            if (!visited[v])
            {
                visited[v] = 1;
                dfs_tree[f.node].push_back(v);
                dfs_tree[v].push_back(f.node);
                numbering[w] = v;
                w++;
                stk.push({v, 0});
            }
        }
        else
        {
            stk.pop();
        }
    }
}

// Iterative trace ear (avoids stack overflow)
void traceEar(vector<vector<int>> &dfs_tree, vector<int> &ear, vector<int> &visited, int start)
{
    stack<int> stk;
    visited[start] = 1;
    stk.push(start);
    while (!stk.empty())
    {
        int v = stk.top();
        stk.pop();
        for (int u : dfs_tree[v])
        {
            if (!visited[u])
            {
                visited[u] = 1;
                ear.push_back(u);
                stk.push(u);
            }
        }
    }
}

// Find non-tree edges and build ear decomposition
void findNonTreeEdges(vector<vector<int>> &g, vector<vector<int>> &dfs_tree, vector<int> &numbering, vector<vector<int>> &ear_decomposition, int n)
{
    // Build a set of tree edges for fast lookup
    set<pair<int,int>> tree_edges;
    for (int u = 0; u < n; u++)
    {
        for (int v : dfs_tree[u])
        {
            if (u < v)
                tree_edges.insert({u, v});
        }
    }

    vector<int> visited(n, 0);

    for (auto u : numbering)
    {
        if (u < 0 || u >= n) continue;
        visited[u] = 1;
        for (int v : g[u])
        {
            int a = min(u, v), b = max(u, v);
            bool isTreeEdge = tree_edges.count({a, b}) > 0;

            if (!isTreeEdge && !visited[v])
            {
                vector<int> ear;
                ear.push_back(u);
                ear.push_back(v);
                traceEar(dfs_tree, ear, visited, v);
                ear_decomposition.push_back(ear);
            }
        }
    }
}

size_t calculateVectorMemory(const vector<vector<int>> &vec)
{
    size_t memory = sizeof(vec);
    for (const auto &inner : vec)
    {
        memory += sizeof(inner);
        memory += inner.capacity() * sizeof(int);
    }
    return memory;
}

int main(int argc, char *argv[])
{
    ios::sync_with_stdio(0);
    cin.tie(0);
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // Start timing
    auto start = chrono::high_resolution_clock::now();

    ifstream infile(argv[1]);
    if (!infile)
    {
        cerr << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }

    int n = 0, m = 0;
    string line;

    // Skip comment lines starting with '#'
    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        // First non-comment line has n and m
        stringstream ss(line);
        if (ss >> n >> m)
            break;
    }

    if (n <= 0)
    {
        cerr << "Error: Could not find valid n and m values in the file" << endl;
        return 1;
    }

    vector<vector<int>> g(n);
    int edges_read = 0;
    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        stringstream ss(line);
        int u, v;
        if (ss >> u >> v)
        {
            if (u >= 0 && u < n && v >= 0 && v < n)
            {
                g[u].push_back(v);
                g[v].push_back(u);
                edges_read++;
            }
        }
    }
    infile.close();

    size_t g_memory = calculateVectorMemory(g);

    vector<vector<int>> dfs_tree(n);
    vector<int> visited(n, 0);
    vector<int> numbering(n);
    dfs(g, dfs_tree, 0, 0, visited, numbering);

    size_t dfs_tree_memory = calculateVectorMemory(dfs_tree);
    size_t visited_memory = sizeof(visited) + visited.capacity() * sizeof(int);
    size_t numbering_memory = sizeof(numbering) + numbering.capacity() * sizeof(int);

    vector<vector<int>> ear_decomposition;
    findNonTreeEdges(g, dfs_tree, numbering, ear_decomposition, n);

    size_t ear_decomposition_memory = calculateVectorMemory(ear_decomposition);
    size_t total_memory = g_memory + dfs_tree_memory + visited_memory + numbering_memory + ear_decomposition_memory;

    // End timing
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    // Extract filename from path
    string filepath = argv[1];
    string filename = filepath;
    size_t pos = filepath.find_last_of("/\\");
    if (pos != string::npos)
        filename = filepath.substr(pos + 1);

    cout << filename << ": " << fixed << setprecision(6) << elapsed.count()
         << " seconds, Memory : " << total_memory << " Bytes"
         << ", Vertices: " << n << ", Edges: " << edges_read << endl;

    return 0;
}
