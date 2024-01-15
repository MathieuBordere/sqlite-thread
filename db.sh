#!/bin/bash

# Check if a parameter for the number of rows is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <number_of_rows>"
    exit 1
fi

num_rows=$1
db_file="benchmark.db"

# Create SQLite database and table
sqlite3 $db_file "CREATE TABLE IF NOT EXISTS benchmark (id INTEGER PRIMARY KEY, text TEXT);"

# Generate 1KB of dummy text
dummy_text=$(head -c 1024 < /dev/urandom | tr -dc 'a-zA-Z0-9')

# Insert data into the table
for (( i=1; i<=num_rows; i++ ))
do
    sqlite3 $db_file "INSERT INTO benchmark (text) VALUES ('$dummy_text');"
done
