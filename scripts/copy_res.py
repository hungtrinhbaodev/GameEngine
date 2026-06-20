import os
import sys
import shutil


def sync_resources(src_dir, dst_dir):
    if not os.path.isdir(src_dir):
        print(f"[res-sync] Source not found: {src_dir}")
        sys.exit(1)

    os.makedirs(dst_dir, exist_ok=True)

    copied = 0
    skipped = 0
    deleted = 0

    # Copy new or changed files from src to dst
    for root, dirs, files in os.walk(src_dir):
        rel_root = os.path.relpath(root, src_dir)
        dst_root = os.path.join(dst_dir, rel_root)
        os.makedirs(dst_root, exist_ok=True)

        for file in files:
            src_file = os.path.join(root, file)
            dst_file = os.path.join(dst_root, file)

            src_mtime = os.path.getmtime(src_file)
            src_size = os.path.getsize(src_file)

            if os.path.exists(dst_file):
                dst_mtime = os.path.getmtime(dst_file)
                dst_size = os.path.getsize(dst_file)
                if src_mtime <= dst_mtime and src_size == dst_size:
                    skipped += 1
                    continue

            shutil.copy2(src_file, dst_file)
            print(f"[res-sync] Copied: {os.path.join(rel_root, file)}")
            copied += 1

    # Delete files in dst that no longer exist in src
    for root, dirs, files in os.walk(dst_dir):
        rel_root = os.path.relpath(root, dst_dir)
        src_root = os.path.join(src_dir, rel_root)

        for file in files:
            src_file = os.path.join(src_root, file)
            dst_file = os.path.join(root, file)
            if not os.path.exists(src_file):
                os.remove(dst_file)
                print(f"[res-sync] Deleted stale: {os.path.join(rel_root, file)}")
                deleted += 1

    print(f"[res-sync] Done — copied: {copied}, skipped: {skipped}, deleted: {deleted}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: copy_res.py <src_res_dir> <dst_res_dir>")
        sys.exit(1)

    sync_resources(sys.argv[1], sys.argv[2])
