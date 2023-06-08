# Команды, чтобы не забыть

----#Конфигурация

cmake --preset debug

cmake --preset release

----#Сборка

cmake --build --preset debug

cmake --build --preset release

----#Запуск

./build/debug/bin/indexer --csv books.csv --index index

./build/debug/bin/searcher --index index

./build/debug/bin/searcher --index index --query "harry potter"

./run.sh --index=index

./build/debug/bin/Tests
