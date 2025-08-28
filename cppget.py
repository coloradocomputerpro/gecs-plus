import os
import re
import argparse
import fnmatch
from pathlib import Path
from typing import List

class ProjectGatherer:
    INCLUDE_ONLY_DIRS = {
        'src', 
        'doc_classes', 
        'include'
    }

    INCLUDE_ONLY_ROOT_FILES = {
        'SConstruct',
        'main.gdextension'
    }

    def __init__(self, root_dir: str, file_types: List[str] = None, include_tree: bool = False, 
                 ignore_patterns: List[str] = None, debug: bool = False, allow_dot_root: bool = False,
                 append_tree: bool = False):
        self.root_dir = Path(root_dir).expanduser().resolve()
        self.project_name = self.root_dir.name
        self.output_file = Path(f"{self.project_name}-project.txt")
        self.file_types = file_types or ["cpp", "h", "hpp", "py", "gdextension"]
        self.include_tree = include_tree
        self.ignore_patterns = ignore_patterns or []
        self.debug = debug
        self.allow_dot_root = allow_dot_root
        self.append_tree = append_tree
        
        if self.debug:
            print(f"Initialized gatherer with:")
            print(f"  Root directory: {self.root_dir}")
            print(f"  Project name: {self.project_name}")
            print(f"  Output file: {self.output_file}")
            print(f"  File types: {self.file_types}")
            print(f"  Include tree: {self.include_tree}")
            print(f"  Ignore patterns: {self.ignore_patterns}")
            print(f"  Allow dot root: {self.allow_dot_root}")
            print(f"  Append tree: {self.append_tree}")
            print(f"  WHITELIST DIRS: {self.INCLUDE_ONLY_DIRS}")
            print(f"  WHITELIST ROOT FILES: {self.INCLUDE_ONLY_ROOT_FILES}")

    def should_skip(self, path: Path) -> bool:
        rel_path = path.relative_to(self.root_dir)
        rel_parts = rel_path.parts

        is_in_root = len(rel_parts) == 1
        is_file = path.is_file()

        if is_file:
            if is_in_root:
                if path.name not in self.INCLUDE_ONLY_ROOT_FILES:
                    if self.debug:
                        print(f"Skipping '{path}': Root file not in whitelist.")
                    return True
            else:
                top_level_dir = rel_parts[0]
                if top_level_dir not in self.INCLUDE_ONLY_DIRS:
                    if self.debug:
                        print(f"Skipping '{path}': Directory '{top_level_dir}' not in whitelist.")
                    return True
        else:
            if not is_in_root:
                top_level_dir = rel_parts[0]
                if top_level_dir not in self.INCLUDE_ONLY_DIRS:
                    if self.debug:
                        print(f"Skipping directory '{path}': Not in whitelist.")
                    return True

        for pattern in self.ignore_patterns:
            if fnmatch.fnmatch(path.name, pattern) or fnmatch.fnmatch(str(rel_path), pattern):
                if self.debug:
                    print(f"Skipping '{path}': Matches ignore pattern '{pattern}'")
                return True
                
        return False

    def gather_files(self):
        if self.debug:
            print(f"\nGathering files with types: {self.file_types}")
        
        files_processed = 0
        files_included = 0
        
        all_matching_files = []
        file_extensions_to_match = tuple(f".{ft}" for ft in self.file_types)
        
        for dirpath, dirnames, filenames in os.walk(self.root_dir, followlinks=False):
            dir_path = Path(dirpath)
            
            filtered_dirnames = []
            for d in dirnames:
                if not self.should_skip(dir_path / d):
                    filtered_dirnames.append(d)
            dirnames[:] = filtered_dirnames

            for filename in filenames:
                file_path = dir_path / filename
                if not self.should_skip(file_path):
                    is_root_file = file_path.name in self.INCLUDE_ONLY_ROOT_FILES and len(file_path.relative_to(self.root_dir).parts) == 1
                    if filename.endswith(file_extensions_to_match) or is_root_file:
                        all_matching_files.append(file_path)

        if self.debug:
            print(f"Found {len(all_matching_files)} whitelisted files matching patterns.")
            if len(all_matching_files) > 0:
                print("First 5 files found:")
                for f in sorted(all_matching_files)[:5]:
                    print(f"  - {f}")
        
        with open(self.output_file, 'w', encoding='utf-8') as outfile:
            for file in sorted(all_matching_files):
                files_processed += 1
                rel_path = file.relative_to(self.root_dir)

                try:
                    with open(file, 'r', encoding='utf-8', errors='ignore') as infile:
                        content = infile.read().strip()
                except Exception as e:
                    print(f"Error processing {file}: {e}")
                    content = ""

                if self.include_tree or content:
                    if self.debug:
                        print(f"Including: {rel_path}")
                    
                    file_type = file.suffix.lstrip('.') if file.suffix else 'file'
                    outfile.write(f"\n#{'='*80}\n# {rel_path} ({file_type})\n#{'='*80}\n\n")
                    if content:
                        outfile.write(content + "\n\n")
                    files_included += 1
            
        if self.debug:
            print(f"\nSummary:")
            print(f"  Files processed: {files_processed}")
            print(f"  Files included: {files_included}")
            print(f"  Output file: {self.output_file}")
            
        return files_included > 0

def main():
    parser = argparse.ArgumentParser(description="A helper utility to collect Godot GDExtension C/C++ project files for LLM analysis using a whitelist or filters")
    parser.add_argument("project_dir", help="Root directory of the project")
    parser.add_argument("--gather", dest="file_types", nargs="+",
                        help="Additional file types to gather (e.g., glsl).")
    parser.add_argument("--ignore", dest="ignore_patterns", nargs="+", default=[],
                        help="Patterns to ignore (e.g., 'test_*.cpp' 'temp/*')")
    parser.add_argument("--debug", action="store_true", help="Enable debug output")
    args = parser.parse_args()

    custom_file_types = ["cpp", "h", "hpp", "gdextension"]
    if args.file_types:
        custom_file_types.extend(args.file_types)

    gatherer = ProjectGatherer(
        args.project_dir, 
        file_types=list(set(custom_file_types)),
        ignore_patterns=args.ignore_patterns,
        debug=args.debug
    )
    
    success = gatherer.gather_files()
    if success:
        print(f"Whitelisted files gathered into {gatherer.output_file}")
    else:
        print(f"No matching files found in the whitelist. Check paths and try --debug for more information.")

if __name__ == "__main__":
    main()