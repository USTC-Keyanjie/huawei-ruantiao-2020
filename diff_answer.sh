git pull

rm -rf ./final
g++ final.cpp -o final -O3 -g -lpthread -fpic

for i in {0..24}
do
    rm -rf test_data_js/$i/result.txt
    echo -
    echo -
    echo -------------   test_data: $i  --------------
    echo -
    ./final $i
    diff test_data_js/$i/result.txt test_data_js/$i/answer.txt
done