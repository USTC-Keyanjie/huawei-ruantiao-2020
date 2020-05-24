git pull

g++ -O3 final.cpp -o final -lpthread -fpic

rm -rf test_data_js/0/result.txt
./final 0
diff test_data_js/0/result.txt test_data_js/0/answer.txt

rm -rf test_data_js/1/result.txt
./final 1
diff test_data_js/1/result.txt test_data_js/1/answer.txt

rm -rf test_data_js/2/result.txt
./final 2
diff test_data_js/2/result.txt test_data_js/2/answer.txt

rm -rf test_data_js/3/result.txt
./final 3
diff test_data_js/3/result.txt test_data_js/3/answer.txt