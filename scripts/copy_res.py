import os
import sys
import shutil
import subprocess

SHADER_EXTENSIONS = (".vert", ".frag")


def find_glslc():
    vulkan_sdk = os.environ.get("VULKAN_SDK")
    if vulkan_sdk:
        for sub_dir in ("Bin", "bin"):
            candidate = os.path.join(vulkan_sdk, sub_dir, "glslc.exe" if os.name == "nt" else "glslc")
            if os.path.isfile(candidate):
                return candidate

    glslc_on_path = shutil.which("glslc")
    if glslc_on_path:
        return glslc_on_path

    print("[shader-compile] glslc not found (checked $VULKAN_SDK and PATH) — can't compile shaders.")
    sys.exit(1)


def compile_shaders(shader_dir):
    if not os.path.isdir(shader_dir):
        return

    glslc_path = None
    compiled = 0
    skipped = 0

    for root, dirs, files in os.walk(shader_dir):
        for file in files:
            if not file.endswith(SHADER_EXTENSIONS):
                continue

            src_file = os.path.join(root, file)
            spv_file = src_file + ".spv"

            if os.path.exists(spv_file) and os.path.getmtime(spv_file) >= os.path.getmtime(src_file):
                skipped += 1
                continue

            if glslc_path is None:
                glslc_path = find_glslc()

            result = subprocess.run([glslc_path, src_file, "-o", spv_file], capture_output=True, text=True)
            if result.returncode != 0:
                print(f"[shader-compile] Failed to compile {os.path.relpath(src_file, shader_dir)}:")
                print(result.stderr)
                sys.exit(1)

            print(f"[shader-compile] Compiled: {os.path.relpath(src_file, shader_dir)}")
            compiled += 1

    print(f"[shader-compile] Done — compiled: {compiled}, skipped: {skipped}")


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

    compile_shaders(os.path.join(sys.argv[1], "shader"))
    sync_resources(sys.argv[1], sys.argv[2])
