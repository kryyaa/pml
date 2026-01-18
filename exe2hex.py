import sys

def file_to_c_hex(data, bytes_per_line=16):
    out = []
    for i, b in enumerate(data):
        out.append(f"0x{b:02X}")
        if i != len(data) - 1:
            out.append(", ")
        if (i + 1) % bytes_per_line == 0:
            out.append("\n")
    return "".join(out)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python exe2hex.py <file.exe> <out.txt>")
        sys.exit(1)

    with open(sys.argv[1], "rb") as f:
        data = f.read()

    hex_text = file_to_c_hex(data)

    with open(sys.argv[2], "w", encoding="utf-8") as f:
        f.write(hex_text)

    print(f"[OK] Saved to {sys.argv[2]}")
