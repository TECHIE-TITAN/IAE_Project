#include <bits/stdc++.h>
#include <chrono>
using namespace std;

// ── DFS: builds parent[], depth[], and collects back edges ──────────────────
// Back edges are stored as (u, v) where v is the ancestor (depth[v] < depth[u])
void dfs(const vector<vector<int>> &g,
         vector<int> &parent,
         vector<int> &depth,
         vector<pair<int,int>> &back_edges,
         int n)
{
    struct Frame { int node; int idx; };
    vector<int> visited(n, 0);
    stack<Frame> stk;

    // Handle disconnected graphs: restart DFS from every unvisited node
    for (int start = 0; start < n; start++)
    {
        if (visited[start]) continue;
        visited[start] = 1;
        depth[start] = 0;
        parent[start] = -1;
        stk.push({start, 0});

        while (!stk.empty())
        {
            Frame &f = stk.top();
            int u = f.node;
            if (f.idx < (int)g[u].size())
            {
                int v = g[u][f.idx++];
                if (v == parent[u]) continue;   // skip the single tree-parent edge
                if (!visited[v])
                {
                    visited[v] = 1;
                    parent[v] = u;
                    depth[v] = depth[u] + 1;
                    stk.push({v, 0});
                }
                else if (depth[v] < depth[u])   // v is a proper ancestor → back edge
                {
                    back_edges.push_back({u, v});
                }
            }
            else
            {
                stk.pop();
            }
        }
    }
}

// ── Chain decomposition ─────────────────────────────────────────────────────
// For each back edge (u → ancestor v), walk parent[] from u up to v.
// A tree edge is claimed by the FIRST (shallowest-covering) back edge that
// touches it; once claimed it won't appear in any later chain.
// This naturally partitions all back edges + tree edges into disjoint chains.
void buildChains(const vector<pair<int,int>> &back_edges,
                 const vector<int> &parent,
                 const vector<int> &depth,
                 vector<vector<int>> &chains,
                 int n)
{
    // chain_id[v] = index of the chain that owns the tree edge (v → parent[v])
    // -1 means unclaimed
    vector<int> chain_id(n, -1);

    // Sort back edges by depth of ancestor (shallowest first) so higher-up
    // back edges get priority — this matches the canonical Schmidt ordering.
    vector<pair<int,int>> sorted_be = back_edges;
    sort(sorted_be.begin(), sorted_be.end(),
         [&](const pair<int,int> &a, const pair<int,int> &b){
             return depth[a.second] < depth[b.second];
         });

    for (auto [u, v] : sorted_be)
    {
        vector<int> chain;
        chain.push_back(u);

        int cur = u;
        // Walk up the DFS tree from u toward v, collecting unclaimed tree edges
        while (cur != v && chain_id[cur] == -1)
        {
            chain_id[cur] = (int)chains.size();
            cur = parent[cur];
            chain.push_back(cur);
        }
        // chain now ends either at v (fresh path) or at a node already in a
        // prior chain (convergence point) — both are valid chain endings.

        if (chain.size() >= 2)          // skip degenerate single-node chains
            chains.push_back(chain);
    }
}

// ── Memory helper ────────────────────────────────────────────────────────────
size_t vecVecMemory(const vector<vector<int>> &v)
{
    size_t m = sizeof(v);
    for (const auto &inner : v)
        m += sizeof(inner) + inner.capacity() * sizeof(int);
    return m;
}

int main(int argc, char *argv[])
{
    ios::sync_with_stdio(0);
    cin.tie(0);

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    auto t0 = chrono::high_resolution_clock::now();

    ifstream infile(argv[1]);
    if (!infile)
    {
        cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    int n = 0, m = 0;
    string line;

    // Skip comment lines
    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        if (ss >> n >> m) break;
    }

    if (n <= 0)
    {
        cerr << "Error: Could not parse n and m\n";
        return 1;
    }

    vector<vector<int>> g(n);
    int edges_read = 0;

    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '#') continue;
        if (edges_read >= m) break;         // honour m — stops on trailing junk
        istringstream ss(line);
        int u, v;
        if (!(ss >> u >> v)) continue;
        if (u < 0 || u >= n || v < 0 || v >= n) continue;
        if (u == v) continue;               // skip self-loops

        // Deduplicate: only add if edge not already present
        // (for huge graphs, swap this for an unordered check or accept duplicates)
        g[u].push_back(v);
        g[v].push_back(u);
        edges_read++;
    }
    infile.close();

    // ── DFS ──────────────────────────────────────────────────────────────────
    vector<int> parent(n, -1);
    vector<int> depth(n, 0);
    vector<pair<int,int>> back_edges;
    back_edges.reserve(edges_read);         // upper bound; avoids realloc

    dfs(g, parent, depth, back_edges, n);

    // ── Chain decomposition ───────────────────────────────────────────────────
    vector<vector<int>> chains;
    buildChains(back_edges, parent, depth, chains, n);

    // ── Memory accounting ─────────────────────────────────────────────────────
    size_t mem_g       = vecVecMemory(g);
    size_t mem_parent  = sizeof(parent)  + parent.capacity()  * sizeof(int);
    size_t mem_depth   = sizeof(depth)   + depth.capacity()   * sizeof(int);
    size_t mem_be      = sizeof(back_edges) + back_edges.capacity() * sizeof(pair<int,int>);
    size_t mem_chains  = vecVecMemory(chains);
    size_t total_mem   = mem_g + mem_parent + mem_depth + mem_be + mem_chains;

    auto t1 = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t1 - t0;

    // Extract filename
    string filepath = argv[1];
    string filename = filepath.substr(filepath.find_last_of("/\\") + 1);

    cout << filename
         << ": "    << fixed << setprecision(6) << elapsed.count() << " seconds"
         << ", Memory: "   << total_mem   << " bytes"
         << ", Vertices: " << n
         << ", Edges: "    << edges_read
         << ", Chains: "   << chains.size()
         << ", Back edges: " << back_edges.size()
         << "\n";

    return 0;
}