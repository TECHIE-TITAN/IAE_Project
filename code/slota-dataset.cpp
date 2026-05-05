#include <bits/stdc++.h>
#include <chrono>
#include <omp.h>
#include <fstream>
#include <sstream>

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

bool exclusion_bfs(const vector<vector<int>> &adj, const vector<int> &L, const vector<int> &P, vector<int> &stamp, int remove_v, int start_w, int lvl_v, int curStamp)
{
    queue<int> Q;
    stamp[start_w] = curStamp;
    Q.push(start_w);
    while (!Q.empty())
    {
        int u = Q.front();
        Q.pop();
        
        if (L[u] <= lvl_v && u != remove_v)
            return true;
            
        for (int nb : adj[u])
        {
            if (nb != remove_v && nb >= 0 && nb < (int)stamp.size() && stamp[nb] != curStamp)
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
        if ((int)adj[i].size() != 2)
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
    
    int start_child = children[0];
    queue<int> Q;
    Q.push(start_child);
    curStamp++;
    stamp[start_child] = curStamp;
    
    int children_reached = 1;
    
    while (!Q.empty())
    {
        int u = Q.front();
        Q.pop();
        for (int v : adj[u])
        {
            if (v == root || stamp[v] == curStamp)
            {
                continue;
            }
            stamp[v] = curStamp;
            Q.push(v);
            
            for (int child : children)
            {
                if (v == child)
                {
                    children_reached++;
                    break;
                }
            }
        }
    }
    
    return children_reached < (int)children.size();
}

int main(int argc, char *argv[])
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    string input_file = argv[1];
    auto start = chrono::high_resolution_clock::now();

    ifstream fin(input_file);
    if (!fin)
    {
        cerr << "Error: could not open " << input_file << "\n";
        return 1;
    }

    string line;
    int n = 0, m = 0;

    while (getline(fin, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        stringstream ss(line);
        if (ss >> n >> m)
            break;
    }

    if (n <= 0)
    {
        cerr << "Error: Could not find valid dimensions\n";
        return 1;
    }

    vector<vector<int>> adj(n);
    int edges_read = 0;
    while (getline(fin, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        stringstream ss(line);
        int u, v;
        if (ss >> u >> v)
        {
            if (u >= 0 && u < n && v >= 0 && v < n)
            {
                adj[u].push_back(v);
                adj[v].push_back(u);
                edges_read++;
            }
        }
    }
    fin.close();

    vector<int> P(n, -2), L(n, -1), stamp(n, 0);
    vector<bool> seen(n, false), isArt_slota(n, false);

    if (is_cycle_graph(adj, n))
    {
        // Cycle graphs have no articulation points.
    }
    else
    {
        for (int root = 0; root < n; root++)
        {
            if (seen[root])
                continue;

            vector<int> comp;
            vector<int> root_children;
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
                        
                        if (u == root)
                        {
                            root_children.push_back(nb);
                        }
                    }
                }
            }
            
            vector<int> root_local_stamp(n, 0);
            int root_local_curStamp = 1;
            if (is_root_articulation_slota(adj, root, root_children, root_local_stamp, root_local_curStamp))
            {
                isArt_slota[root] = true;
            }

            // Explicitly set the number of execution threads to 16
            omp_set_num_threads(16);

            #pragma omp parallel
            {
                // Allocated ONCE per thread, not per loop iteration
                vector<int> thread_stamp(n, 0);
                int thread_curStamp = 1;
                vector<int> local_children; 

                // Dynamic scheduling handles variable execution paths of exclusion_bfs
                #pragma omp for schedule(dynamic, 4)
                for (size_t i = 0; i < comp.size(); i++)
                {
                    int v = comp[i];
                    if (v == root)
                        continue;

                    local_children.clear();
                    for (int nb : adj[v])
                    {
                        if (P[nb] == v)
                        {
                            local_children.push_back(nb);
                        }
                    }
                    if (local_children.empty())
                        continue;
                        
                    bool is_articulation = false;
                    for (int w : local_children)
                    {
                        // Safely increment stamp
                        thread_curStamp++; 
                        if (exclusion_bfs(adj, L, P, thread_stamp, v, w, L[v], thread_curStamp))
                        {
                            // It managed to escape, check next child
                        }
                        else
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
    }

    int art_count = count(isArt_slota.begin(), isArt_slota.end(), true);

    size_t adj_memory = calculateVectorMemory(adj);
    size_t P_memory = sizeof(P) + P.capacity() * sizeof(int);
    size_t L_memory = sizeof(L) + L.capacity() * sizeof(int);
    size_t stamp_memory = sizeof(stamp) + stamp.capacity() * sizeof(int);
    size_t seen_memory = sizeof(seen) + seen.capacity() * sizeof(bool);
    size_t isArt_slota_memory = sizeof(isArt_slota) + isArt_slota.capacity() * sizeof(bool);
    size_t total_memory = adj_memory + P_memory + L_memory + stamp_memory + seen_memory + isArt_slota_memory;

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    string filepath = argv[1];
    string filename = filepath;
    size_t pos = filepath.find_last_of("/\\");
    if (pos != string::npos)
        filename = filepath.substr(pos + 1);

    cout << filename << ": " << fixed << setprecision(6) << elapsed.count()
         << " seconds, Memory : " << total_memory << " Bytes"
         << ", Vertices: " << n << ", Edges: " << edges_read
         << ", Articulation Points: " << art_count << endl;

    return 0;
}