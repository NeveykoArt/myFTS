----#Configure

cmake --preset debug

cmake --preset release

----#Build

cmake --build --preset debug

cmake --build --preset release

----#PowerOn, Example

./build/debug/bin/indexer --csv books.csv --index index

./build/debug/bin/searcher --index index

./build/debug/bin/searcher --index index --query "harry potter"

./run.sh --index=index

./build/debug/bin/Tests
