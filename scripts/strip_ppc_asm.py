import re
import glob

pattern = re.compile(r'\s*asm\("(?:r|fr)\d+"\)')

total_files = 0
for path in glob.glob("src/**/*.cpp", recursive=True):
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    if pattern.search(content):
        new_content = pattern.sub("", content)
        with open(path, "w", encoding="utf-8") as f:
            f.write(new_content)
        total_files += 1
        print("Updated:", path)

print(f"Total files updated: {total_files}")
