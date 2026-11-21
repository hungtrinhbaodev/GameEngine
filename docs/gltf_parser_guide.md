# GLB Parser Guide — `res/cat 7.glb`

Personal working notes for writing a from-scratch (no glTF library) `.glb` parser.

**File facts confirmed from the actual bytes of `res/cat 7.glb`:**
- Magic: `"glTF"`, version `2`, total size `4,822,344` bytes.
- Chunk 0: `20,876` bytes, type `"JSON"`, starting at byte offset 20.
- A second chunk follows (type `"BIN\0"`) holding all binary buffer data.

**Byte-by-byte layout of the header + chunk 0, from the raw hex dump:**
- Bytes 0-3: `"glTF"` magic.
- Bytes 4-7: version = `2` (little-endian).
- Bytes 8-11: total file length = `0x00499548` = 4,822,344 — matches the file exactly.
- Bytes 12-15: chunk 0 length = `0x0000518C` = 20,876 bytes.
- Bytes 16-19: chunk 0 type = `"JSON"`.
- Byte 20 onward: the JSON text itself, starting `{"asset": {"version": "2.0", "generator": "C...`.

## Step 1 — Container parsing (start here, no library needed)

```cpp
uint32_t magic       = read_u32_le(data, 0);   // "glTF" as bytes
uint32_t version     = read_u32_le(data, 4);   // should be 2
uint32_t total_size  = read_u32_le(data, 8);

uint32_t offset = 12;
while (offset < total_size) {
    uint32_t chunk_len  = read_u32_le(data, offset);
    uint32_t chunk_type = read_u32_le(data, offset + 4);   // 0x4E4F534A = "JSON", 0x004E4942 = "BIN\0"
    const uint8_t* chunk_data = data + offset + 8;
    // handle chunk_data[0..chunk_len)
    offset += 8 + chunk_len;
}
```

`read_u32_le` always assembles bytes little-endian by shifting — no host-endianness check needed,
since glTF's binary format is always little-endian regardless of what machine reads it:

```cpp
uint32_t read_u32_le(const uint8_t* bytes, uint32_t offset) {
    return uint32_t(bytes[offset])
         | (uint32_t(bytes[offset + 1]) << 8)
         | (uint32_t(bytes[offset + 2]) << 16)
         | (uint32_t(bytes[offset + 3]) << 24);
}
```

## Step 2 — JSON parsing (open decision, not yet settled)

Decide: write a full custom JSON parser, or pull in a small single-header JSON library
(not a glTF library, just JSON text parsing — e.g. a single `nlohmann::json` header)?
This decides how much of "no library" step 2 actually is.

## Step 3 — Accessors and buffer views

Once the JSON is parsed into data, walk:
```
meshes[].primitives[].attributes.POSITION  -> accessor index
accessors[index]                            -> { bufferView, componentType, count, type }
bufferViews[bufferView]                     -> { buffer, byteOffset, byteLength, byteStride }
```
`componentType` (e.g. `5126` = float) and `type` (e.g. `"VEC3"`) tell you how to interpret the raw bytes.

## Step 4 — Reading raw vertex/index bytes

With the resolved byte offset into the BIN chunk, read each attribute the same way as the header —
raw byte offsets, no library:

```cpp
const float* positions = (const float*)(bin_chunk_data + bufferView.byteOffset + accessor.byteOffset);
```

## Step 5 — Wire into the engine

Feed parsed positions/normals/UVs/indices into the engine's own `Vertex` struct, then
`Static_Buffer::upload_data`, same pattern as the triangle test data in `vk_core.cpp`.

## Endianness notes (from working through this)

- Endianness only describes **byte order** in a multi-byte value — it says nothing about bit
  order within a single byte, and nothing about how `<<`/`>>`/`&`/`|` behave (those are always
  the same on every machine).
- This dev machine (x86/x64) is little-endian, confirmed both by decoding the real header bytes
  and by the classic `value = 1; check first byte` runtime check.
- Manually assembling multi-byte values with shifts (`read_u32_le` above) sidesteps needing to
  detect host endianness at all — it's correct on any machine, since the shift math is universal
  and only the *choice* of which byte is least-significant encodes "this file is little-endian."
