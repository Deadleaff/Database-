#!/bin/bash

for i in {1..7}; do
> temp/output$i.txt
    cat > temp/input$i.txt << EOF
INSERT name = Client${i}_A, group = $((100 + i)), rating = 4.5
INSERT name = Client${i}_B, group = $((100 + i)), rating = 3.5
INSERT name = Client${i}_C, group = $((200 + i)), rating = 5.0
INSERT name = Client${i}_D, group = $((200 + i)), rating = 2.5
SELECT
SELECT group = $((100 + i))
SELECT rating = 4.0-5.0
SELECT name = Client${i}_A
PRINT name, group
PRINT name, rating
PRINT group, rating
PRINT
REMOVE name = Client${i}_B
SELECT
PRINT
REMOVE group = $((100 + i))
SELECT
PRINT
REMOVE rating = 4.0-5.0
SELECT
SELECT
PRINT
q
EOF
done

parallel -a test/parallel_input.txt

echo Тесты пройдены
