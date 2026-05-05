"""
Run Jen-Schmidt and Slota-Madduri algorithms on all graph types in the dataset.
Generates 2 result files per graph category (one per algorithm).
"""
import os
import subprocess
import sys
import time

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

DATASET_BASE = "dataset"
RESULTS_DIR = "results"
TIMEOUT_SECONDS = 300  # 5 minute timeout per graph

CATEGORIES = [
    "dense",
    "sparse",
    "small",
    "large",
    "tree_like",
    "highly_connected",
    "real_world",
]

def compile_code():
    """Compile the adapted C++ source files."""
    print("=" * 60)
    print("Compiling C++ sources...")
    print("=" * 60)

    try:
        subprocess.run(
            ["g++", "Jen-Schmidt-dataset.cpp", "-o", "jenschmidt_dataset", "-std=c++17", "-O2"],
            check=True,
            capture_output=True,
            text=True,
        )
        print("  [OK] Jen-Schmidt-dataset.cpp -> jenschmidt_dataset")
    except subprocess.CalledProcessError as e:
        print(f"  [FAIL] Jen-Schmidt compilation failed:\n{e.stderr}")
        sys.exit(1)

    try:
        subprocess.run(
            ["g++", "slota-dataset.cpp", "-o", "slota_dataset", "-std=c++17", "-O2"],
            check=True,
            capture_output=True,
            text=True,
        )
        print("  [OK] slota-dataset.cpp -> slota_dataset")
    except subprocess.CalledProcessError as e:
        print(f"  [FAIL] Slota compilation failed:\n{e.stderr}")
        sys.exit(1)

    print("Compilation successful.\n")


def get_graph_files(category):
    """Get all .txt graph files in a category directory (excluding README)."""
    cat_dir = os.path.join(DATASET_BASE, category)
    if not os.path.isdir(cat_dir):
        print(f"  Warning: Directory {cat_dir} not found, skipping.")
        return []
    files = []
    for f in sorted(os.listdir(cat_dir)):
        if f.endswith(".txt") and f.lower() != "readme.txt":
            files.append(os.path.join(cat_dir, f))
    return files


def run_algorithm(exe_name, graph_file, timeout=TIMEOUT_SECONDS):
    """Run an algorithm on a graph file and return the output line."""
    try:
        result = subprocess.run(
            [f"./{exe_name}", graph_file],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        if result.returncode != 0:
            filename = os.path.basename(graph_file)
            return f"{filename}: ERROR - {result.stderr.strip()}\n"
        return result.stdout
    except subprocess.TimeoutExpired:
        filename = os.path.basename(graph_file)
        return f"{filename}: TIMEOUT after {timeout}s\n"
    except Exception as e:
        filename = os.path.basename(graph_file)
        return f"{filename}: ERROR - {str(e)}\n"


def main():
    compile_code()

    os.makedirs(RESULTS_DIR, exist_ok=True)

    total_files = 0
    total_errors = 0

    for category in CATEGORIES:
        print("=" * 60)
        print(f"Category: {category}")
        print("=" * 60)

        graph_files = get_graph_files(category)
        if not graph_files:
            print(f"  No graph files found in {category}/\n")
            continue

        print(f"  Found {len(graph_files)} graph files")

        jen_results = []
        slota_results = []

        for gf in graph_files:
            fname = os.path.basename(gf)
            print(f"  Processing {fname}...", end=" ", flush=True)

            # Run Jen-Schmidt
            jen_out = run_algorithm("jenschmidt_dataset", gf)
            jen_results.append(jen_out)

            # Run Slota-Madduri
            slota_out = run_algorithm("slota_dataset", gf)
            slota_results.append(slota_out)

            # Show brief status
            jen_ok = "ERROR" not in jen_out and "TIMEOUT" not in jen_out
            slota_ok = "ERROR" not in slota_out and "TIMEOUT" not in slota_out
            status = f"Jen={'OK' if jen_ok else 'FAIL'}, Slota={'OK' if slota_ok else 'FAIL'}"
            print(status)

            total_files += 1
            if not jen_ok or not slota_ok:
                total_errors += 1

        # Write results
        jen_file = os.path.join(RESULTS_DIR, f"{category}_jen.txt")
        with open(jen_file, "w") as f:
            for line in jen_results:
                f.write(line)
        print(f"  -> Saved {jen_file}")

        slota_file = os.path.join(RESULTS_DIR, f"{category}_slota.txt")
        with open(slota_file, "w") as f:
            for line in slota_results:
                f.write(line)
        print(f"  -> Saved {slota_file}")
        print()

    print("=" * 60)
    print(f"DONE! Processed {total_files} graph files across {len(CATEGORIES)} categories.")
    if total_errors > 0:
        print(f"  {total_errors} file(s) had errors or timeouts.")
    print(f"Results saved to {RESULTS_DIR}/")
    print("=" * 60)


if __name__ == "__main__":
    main()
