void save_fwrite(const string &resultFile, int &ansCnt, vector<string> &idsComma, vector<string> &idsLF, vector<vector<vector<int> > > &results) {
#ifdef TEST
	clock_t write_time = clock();
	printf("Total Loops %d\n", ansCnt);
#endif
	//copy the results into str_res and get the length of results
	char *str_res;
	str_res = (char *)malloc(1024*1024*512);// 512M space

	char buf[1024];
	int idx = sprintf(buf, "%d\n", ansCnt);
	buf[idx] = '\0';
	memcpy(str_res, buf, idx);
	int length = idx;
	for (int i = 0; i < 5; ++i) {
		for (vector<int> &single_result : results[i]) {
			for (int j = 0; j < i + 2; j++) {
				const char* res = idsComma[single_result[j]].c_str();
				memcpy(str_res+length,res,strlen(res));
				length += strlen(res);
			}
			const char* res = idsLF[single_result[i + 2]].c_str();
			memcpy(str_res+length,res,strlen(res));
			length += strlen(res);
		}
	}
	//create and open the resultFile
	int fd=open(resultFile.c_str(), O_RDWR|O_CREAT);
	if(fd < 0) {
		printf("open fail\n");
		exit(1);
	}
	//allocate space
	int f_ret = ftruncate(fd, length);
	if (-1 == f_ret) {
		printf("ftruncate fail\n");
		exit(1);
	}
	int count = length/Block_LEN;
	int remain = length - count*Block_LEN;
	for(int i=0; i<count; ++i) {
		char *const address = (char *)mmap(NULL, Block_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fd, Block_LEN*i);
		if (MAP_FAILED == address) {
			printf("mmap fail\n");
			exit(1);
		}
		memcpy(address, str_res+Block_LEN*i, Block_LEN);
		int mun_ret = munmap(address, Block_LEN);
		if (-1 == mun_ret) {
			printf("munmap fail\n");
			exit(1);
		}
	}
	char *const address = (char *)mmap(NULL, remain, PROT_READ|PROT_WRITE, MAP_SHARED, fd, Block_LEN*count);
	if (MAP_FAILED == address) {
		printf("mmap fail\n");
		exit(1);
	}
	memcpy(address, str_res+length-remain, remain);
	int mun_ret = munmap(address, remain);
	if (-1 == mun_ret) {
		printf("munmap fail\n");
		exit(1);
	}
	close(fd);

#ifdef TEST
	cout << "Output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}
