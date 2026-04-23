#include <bits/stdc++.h>
#include <iostream>
#ifdef __linux__
#include <sys/resource.h>
#endif
using namespace std;

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

bool exclusion_bfs(const vector<vector<int>> &adj, const vector<int> &L, const vector<int> &P,vector<int> &stamp, int remove_v, int start_w, int lvl_v, int curStamp)
{
    queue<int> Q;
    stamp[start_w] = curStamp;
    Q.push(start_w);
    while (!Q.empty())
    {
        int u = Q.front();
        Q.pop();
        if ((L[u] <= lvl_v && u != remove_v) || P[u] == remove_v)
            return true;
        for (int nb : adj[u])
        {
            if (nb != remove_v && nb >= 0 && nb < stamp.size() && stamp[nb] != curStamp)
            {
                stamp[nb] = curStamp;
                Q.push(nb);
            }
        }
    }
    return false;
}

bool is_cycle_graph(const vector<vector<int>> &adj, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (adj[i].size() != 2)
        {
            return false;
        }
    }
    vector<bool> visited(n, false);
    queue<int> q;
    q.push(0);
    visited[0] = true;
    int count = 1;
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        for (int v : adj[u])
        {
            if (!visited[v])
            {
                visited[v] = true;
                q.push(v);
                count++;
            }
        }
    }
    return count == n;
}

bool is_root_articulation_slota(const vector<vector<int>> &adj, int root, const vector<int> &children, vector<int> &stamp, int &curStamp)
{
    if (children.size() <= 1)
    {
        return false;
    }
    for (size_t i = 0; i < children.size(); i++)
    {
        int start_child = children[i];
        queue<int> Q;
        Q.push(start_child);
        curStamp++;
        stamp[start_child] = curStamp;
        bool can_reach_other_children = false;
        while (!Q.empty() && !can_reach_other_children)
        {
            int u = Q.front();
            Q.pop();
            for (int v : adj[u])
            {
                if (v == root || stamp[v] == curStamp)
                {
                    continue;
                }
                for (size_t j = 0; j < children.size(); j++)
                {
                    if (j != i && v == children[j])
                    {
                        can_reach_other_children = true;
                        break;
                    }
                }
                stamp[v] = curStamp;
                Q.push(v);
                if (can_reach_other_children)
                {
                    break;
                }
            }
        }
        if (!can_reach_other_children)
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string input_file = "graph.txt";
    if (argc > 1)
    {
        input_file = argv[1];
    }
    ifstream fin(input_file);
    if (!fin)
    {
        cerr << "Error: could not open " << input_file << "\n";
        return 1;
    }
    string line;
    int n = 0, m = 0;
    bool dimensions_read = false;
    while (getline(fin, line))
    {
        if (line.empty() || line[0] == '%')
        {
            continue;
        }
        if (!dimensions_read)
        {
            stringstream ss(line);
            int rows, cols, nnz;
            ss >> rows >> nnz>> cols;
            n = rows;
            m = cols;
            dimensions_read = true;
            break;
        }
    }
    cout << n << " " << m << " ";
    vector<vector<int>> adj(n);
    int u, v;
    double value;
    while (fin >> u >> v >> value)
    {
        u--;
        v--;
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    fin.close();
    vector<int> P(n, -2), L(n, -1), stamp(n, 0);
    vector<bool> seen(n, false), isArt_slota(n, false);
    int curStamp = 1;
    if (is_cycle_graph(adj, n))
    {
        return 0;
    }
    else
    {
        for (int root = 0; root < n; root++)
        {
            if (seen[root])
                continue;
            vector<int> comp;
            queue<int> Q;
            Q.push(root);
            seen[root] = true;
            P[root] = -1;
            L[root] = 0;
            comp.push_back(root);
            while (!Q.empty())
            {
                int u = Q.front();
                Q.pop();
                for (int nb : adj[u])
                {
                    if (!seen[nb])
                    {
                        seen[nb] = true;
                        P[nb] = u;
                        L[nb] = L[u] + 1;
                        Q.push(nb);
                        comp.push_back(nb);
                    }
                }
            }
            vector<int> root_children;
            for (int v = 0; v < n; v++)
            {
                if (P[v] == root)
                {
                    root_children.push_back(v);
                }
            }
            if (is_root_articulation_slota(adj, root, root_children, stamp, curStamp))
            {
                isArt_slota[root] = true;
            }
            for (int v : comp)
            {
                if (v == root)
                    continue;
                bool is_articulation = false;
                vector<int> children;
                for (int nb : adj[v])
                {
                    if (P[nb] == v)
                    {
                        children.push_back(nb);
                    }
                }
                if (children.empty())
                    continue;
                for (int w : children)
                {
                    bool can_reach_ancestor = exclusion_bfs(adj, L, P,stamp, v, w, L[v], ++curStamp);
                    if (!can_reach_ancestor)
                    {
                        is_articulation = true;
                        break;
                    }
                }
                if (is_articulation)
                {
                    isArt_slota[v] = true;
                }
            }
        }
    }
    int art_count = count(isArt_slota.begin(), isArt_slota.end(), true);
    // if (art_count > 0)
    // {
    //     cout << "Found " << art_count << " articulation points. Graph is not biconnected.\n";
    //     // Print the articulation points
    //     // cout << "Articulation points: ";
    //     // for (int i = 0; i < n; i++) {
    //     //     if (isArt_slota[i]) {
    //     //         cout << i << " ";
    //     //     }
    //     // }
    //     // cout << "\n";
    // }
    // else
    // {
    //     cout << "No articulation points found. Graph is biconnected.\n";
    // }
    size_t adj_memory = calculateVectorMemory(adj);
    size_t P_memory = sizeof(P) + P.capacity() * sizeof(int);
    size_t L_memory = sizeof(L) + L.capacity() * sizeof(int);
    size_t stamp_memory = sizeof(stamp) + stamp.capacity() * sizeof(int);
    size_t seen_memory = sizeof(seen) + seen.capacity() * sizeof(bool);
    size_t isArt_slota_memory = sizeof(isArt_slota) + isArt_slota.capacity() * sizeof(bool);
    size_t total_memory = adj_memory + P_memory + L_memory + stamp_memory + seen_memory + isArt_slota_memory;
    cout << total_memory << endl;
    return 0;
}