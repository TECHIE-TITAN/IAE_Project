import os
import time
import subprocess
import csv
import urllib.request
import tarfile
import shutil
import glob
import collections

# Change to the directory where the script is located so paths resolve correctly
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Ensure directories exist (from inside code/)
os.makedirs("Graphs", exist_ok=True)
os.makedirs("../Plots", exist_ok=True)

def parse_target_datasets():
    """Extract unique dataset names from sample_results text files."""
    targets = set()
    for res_file in glob.glob("sample_results/*.txt"):
        with open(res_file, "r") as f:
            for line in f:
                if ":" in line and ".mtx" in line:
                    ds_name = line.split(":")[0].strip().replace(".mtx", "")
                    targets.add(ds_name)
    return list(targets)

def get_official_urls(target_names):
    """Fetch the SuiteSparse index to construct official download URLs."""
    index_url = "http://sparse-files.engr.tamu.edu/files/ssstats.csv"
    urls = []
    
    print("Fetching SuiteSparse index to map names to groups...")
    try:
        req = urllib.request.urlopen(index_url)
        lines = req.read().decode('utf-8').split('\n')
        # Skip the first two header rows
        for line in lines[2:]:
            if line.strip():
                parts = line.split(',')
                group = parts[0]
                name = parts[1]
                if name in target_names:
                    urls.append(f"http://sparse-files.engr.tamu.edu/MM/{group}/{name}.tar.gz")
    except Exception as e:
        print(f"Failed to fetch index: {e}")
        
    return urls

# Start with a baseline fallback if parsing fails, otherwise it will dynamically populate
datasets = [
    "http://sparse-files.engr.tamu.edu/MM/Newman/karate.tar.gz",
    "http://sparse-files.engr.tamu.edu/MM/SNAP/email-Eu-core.tar.gz",
    "http://sparse-files.engr.tamu.edu/MM/SNAP/com-Amazon.tar.gz",
]

def download_and_extract(url, extract_to="Graphs"):
    filename = url.split("/")[-1]
    tar_path = os.path.join(extract_to, filename)
    name_without_ext = filename.split(".")[0]

    # Check if a .mtx file for this dataset already exists
    already_exists = any(name_without_ext in f and f.endswith(".mtx") for f in os.listdir(extract_to))
    
    if not already_exists:
        print(f"Downloading {filename}...")
        try:
            urllib.request.urlretrieve(url, tar_path)
            print(f"Extracting {filename}...")
            with tarfile.open(tar_path, "r:gz") as tar:
                tar.extractall(path=extract_to)
            os.remove(tar_path)
            
            # Find the actual .mtx file and move it up, renaming it
            extracted_folder = os.path.join(extract_to, name_without_ext)
            for root, dirs, files in os.walk(extracted_folder):
                for file in files:
                    if file.endswith(".mtx"):
                        src = os.path.join(root, file)
                        dst = os.path.join(extract_to, f"{name_without_ext}.mtx")
                        os.rename(src, dst)
            
            # Cleanup the extracted folder
            shutil.rmtree(extracted_folder, ignore_errors=True)
        except Exception as e:
            print(f"Failed to download/extract {filename}: {e}")
            if os.path.exists(tar_path):
                os.remove(tar_path)
    else:
        print(f"Dataset {name_without_ext} already exists.")

def get_datasets():
    target_names = parse_target_datasets()
    if target_names:
        official_urls = get_official_urls(target_names)
        if official_urls:
            global datasets
            datasets = official_urls

    print(f"Setting up {len(datasets)} datasets...")
    for url in datasets:
        download_and_extract(url)

def compile_code():
    print("Compiling C++ sources...")
    try:
        subprocess.run(["g++", "slota.cpp", "-o", "slota", "-std=c++17", "-O3"], check=True)
        subprocess.run(["g++", "Jen-Schmidt.cpp", "-o", "jenschmidt", "-std=c++17", "-O3"], check=True)
        print("Compilation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed: {e}")
        exit(1)

def run_experiment(exe_name, mtx_file):
    start_time = time.time()
    try:
        result = subprocess.run(
            [f"./{exe_name}", mtx_file],
            capture_output=True,
            text=True,
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running {exe_name} on {mtx_file}: {e.stderr}")
        return None, None, None, None

    end_time = time.time()
    exec_time = end_time - start_time
    
    # Parse last two lines which may contain logs. 
    # Usually: "123 456 \n 789" => "n m " and "memory" OR "n m memory" on one line
    # Depending on how the output buffer flushed
    output = result.stdout.strip().replace("\n", " ").split()
    if output:
        try:
            # We expect the last 3 tokens to be n, m, total_memory
            n = int(output[-3])
            m = int(output[-2])
            mem = int(output[-1])
            return exec_time, n, m, mem
        except Exception as e:
            print(f"Failed parsing output from {exe_name} on {mtx_file}. Output tokens: {output[-5:]}...")
    return None, None, None, None

def main():
    get_datasets()
    compile_code()

    mtx_files = [os.path.join("Graphs", f) for f in os.listdir("Graphs") if f.endswith(".mtx")]
    if not mtx_files:
        print("No .mtx files found in Graphs/ to process.")
        return

    csv_file = "results.csv"
    print(f"\nRunning experiments and saving to {csv_file}")
    with open(csv_file, 'w', newline='') as f:
        writer = csv.writer(f)
        # 5.4 Execution Flow: Structured CSV format
        writer.writerow(["Dataset", "Vertices", "Edges", "Algorithm", "Time(s)", "Memory(Bytes)"])

        for mtx in mtx_files:
            dataset_name = os.path.basename(mtx)
            print(f"Processing {dataset_name}...")
            
            # Run slota
            time_s, n, m, mem_s = run_experiment("slota", mtx)
            if time_s is not None:
                writer.writerow([dataset_name, n, m, "Slota-Madduri", time_s, mem_s])
            
            # Run jen-schmidt
            time_j, n, m, mem_j = run_experiment("jenschmidt", mtx)
            if time_j is not None:
                writer.writerow([dataset_name, n, m, "Jen-Schmidt", time_j, mem_j])
                
    print(f"\nExperiment complete. Results saved to {csv_file}")
    
    # 5.5 Result Visualization
    try:
        import pandas as pd
        import matplotlib.pyplot as plt
        
        df = pd.read_csv("results.csv")
        
        if len(df) == 0:
            print("No data to plot.")
            return

        # Ensure numeric sorts for proper lines
        df = df.sort_values(by="Edges")

        slota_df = df[df["Algorithm"] == "Slota-Madduri"]
        jen_df = df[df["Algorithm"] == "Jen-Schmidt"]

        # Plot 1: Execution Time vs Edges
        plt.figure(figsize=(10, 6))
        plt.plot(slota_df["Edges"], slota_df["Time(s)"], marker='o', label="Slota-Madduri")
        plt.plot(jen_df["Edges"], jen_df["Time(s)"], marker='x', label="Jen-Schmidt")
        plt.xlabel("Number of Edges")
        plt.ylabel("Execution Time (Seconds)")
        plt.title("Execution Time vs. Number of Edges")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.savefig("../Plots/time_vs_edges.png")
        print("Saved time vs edges plot to ../Plots/time_vs_edges.png")

        # Plot 2: Memory vs Edges
        plt.figure(figsize=(10, 6))
        plt.plot(slota_df["Edges"], slota_df["Memory(Bytes)"] / (1024*1024), marker='o', label="Slota-Madduri")
        plt.plot(jen_df["Edges"], jen_df["Memory(Bytes)"] / (1024*1024), marker='x', label="Jen-Schmidt")
        plt.xlabel("Number of Edges")
        plt.ylabel("Memory Usage (MB)")
        plt.title("Memory Usage vs. Number of Edges")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.savefig("../Plots/mem_vs_edges.png")
        print("Saved memory vs edges plot to ../Plots/mem_vs_edges.png")

    except ImportError:
        print("\npandas or matplotlib not installed. Skipping plotting.")
        print("Install them via: pip3 install pandas matplotlib")

if __name__ == "__main__":
    main()
