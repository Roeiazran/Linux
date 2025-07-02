
This program processes read and write requests from a file (i.e requests.txt) and modifies a data file (i.e data.txt) accordingly. The implementation ensures that writes insert data without overwriting existing content and reads retrieve the correct portion of the file.

Features

Reads specific ranges from data.txt and saves the result to read_results.txt.
Writes new data at a given offset while preserving the rest of the file.
Processes requests from input file instead of user input.
Handles edge cases such as invalid offsets and maintaining file integrity.
Ensures data persistence with proper file operations (lseek, read, write).
