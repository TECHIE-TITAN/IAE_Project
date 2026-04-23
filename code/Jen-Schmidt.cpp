#include <bits/stdc++.h>
using namespace std;

void dfs(vector<vector<int>> &g, vector<vector<int>> &dfs_graph, int u, int w, vector<int> &visited, vector<int> &numbering)
{
    // cout<< "DFS: " << u << endl;
    numbering[w] = u;
    visited[u] = 1;
    for (int v : g[u])
    {
        if (!visited[v])
        {
            dfs_graph[v].push_back(u);
            dfs(g, dfs_graph, v, w + 1, visited, numbering);
        }
    }
}

void traceEar(vector<vector<int>> &dfs_graph, vector<int> &ear, vector<int> &visited, int v)
{
    visited[v] = 1;
    for (int u : dfs_graph[v])
    {
        if (!visited[u])
        {
            ear.push_back(u);
            traceEar(dfs_graph, ear, visited, u);
        }
    }
}
void findNonTreeEdges(vector<vector<int>> &g, vector<vector<int>> &dfs_graph, vector<int> &numbering, vector<vector<int>> &ear_decomposition)
{
    vector<int> visited(g.size(), 0);

    for (auto u : numbering)
    {
        visited[u] = 1;
        for (int v : g[u])
        {
            bool isTreeEdge = false;
            for (int w : dfs_graph[v])
            {
                if (w == u)
                {
                    isTreeEdge = true;
                    break;
                }
            }
            if (!isTreeEdge)
            {
                for (int w : dfs_graph[u])
                {
                    if (w == v)
                    {
                        isTreeEdge = true;
                        break;
                    }
                }
            }
            dfs_graph[u].push_back(v);
            dfs_graph[v].push_back(u);
            vector<int> ear;
            if (!isTreeEdge)
            {
                // cout<< "Ear: " << u << " " << v << endl;
                ear.push_back(u);
                ear.push_back(v);
                traceEar(dfs_graph, ear, visited, v);
                ear_decomposition.push_back(ear);
            }
        }
    }
}

size_t calculateVectorMemory(const vector<vector<int>> &vec)
{
    size_t memory = sizeof(vec); // Size of the vector object itself

    // Add the size of each internal vector
    for (const auto &inner : vec)
    {
        memory += sizeof(inner);                  // Size of the inner vector object
        memory += inner.capacity() * sizeof(int); // Size of the allocated storage
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
    ifstream infile(argv[1]);
    if (!infile)
    {
        cerr << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }
    int n = 0, m = 0;
    string line;
    bool foundNM = false;
    while (getline(infile, line))
    {
        stringstream ss(line);
        int val1, val2,val3;
        if (ss >> val1 >> val3>>val2)
        {
            n = val1;
            m = val2;
            foundNM = true;
            break;
        }
    }
    if (!foundNM)
    {
        cerr << "Error: Could not find valid n and m values in the file" << endl;
        return 1;
    }
    cout << n << " " << m <<" " ;
    vector<vector<int>> g(n + 1);
    for (int i = 0; i < m; i++)
    {
        string edge_line;
        if (!getline(infile, edge_line))
        {
            cerr << "Error: Could not read all " << m << " edges from the file" << endl;
            return 1;
        }

        stringstream ss(edge_line);
        int u, v;
        float weight;
        if (ss >> u >> v)
        {
            g[u - 1].push_back(v - 1);
            g[v - 1].push_back(u - 1);
        }
        else
        {
            cerr << "Error: Invalid edge format at line " << i + 1 << endl;
            return 1;
        }
    }
    infile.close();
    size_t g_memory = calculateVectorMemory(g);
    vector<vector<int>> dfs_graph(n + 1);
    vector<int> visited(n + 1, 0);
    vector<int> numbering(n + 1);
    dfs(g, dfs_graph, 0, 0, visited, numbering);
    size_t dfs_graph_memory = calculateVectorMemory(dfs_graph);
    size_t visited_memory = sizeof(visited) + visited.capacity() * sizeof(int);
    size_t numbering_memory = sizeof(numbering) + numbering.capacity() * sizeof(int);
    vector<vector<int>> ear_decomposition;
    findNonTreeEdges(g, dfs_graph, numbering, ear_decomposition);
    size_t ear_decomposition_memory = calculateVectorMemory(ear_decomposition);
    size_t total_memory = g_memory + dfs_graph_memory + visited_memory + numbering_memory + ear_decomposition_memory;
    cout << total_memory << endl;
    return 0;
}