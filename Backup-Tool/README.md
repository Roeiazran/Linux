This project is a Backup Tool that recursively copies a directory and its contents while preserving symlinks and file permissions. Instead of copying the actual content of regular files, the tool creates hard links to maintain storage efficiency.

Features

Recursively copies all files and directories.
Preserves symbolic links (does not copy the actual file but recreates the symlink).
Creates hard links instead of duplicating file content (for regular files).
Maintains file permissions and attributes.

Conre conceps: Files.
