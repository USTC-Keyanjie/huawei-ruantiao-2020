// mmap不使用memcpy
// void save_results(const string &resultFile, vector<string> &idsComma, vector<string> &idsLF)
// {
// #ifdef TEST
//     clock_t write_time = clock();
//     printf("Total Loops %d\n", res_count);
// #endif

//     //create and open the resultFile
//     int fd = open(resultFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
//     if (fd < 0)
//     {
//         printf("open fail\n");
//         exit(1);
//     }

//     char buf[1024];
//     int idx = sprintf(buf, "%d\n", res_count);
//     buf[idx] = '\0';

//     int str_len = idx;

//     for (int res_len = 3; res_len <= 7; ++res_len)
//     {
//         for (int line = 0; line < num_res[res_len - 3]; ++line)
//         {
//             for (int id = 0; id < res_len - 1; ++id)
//             {
//                 const char *res = idsComma[results[res_len - 3][line * res_len + id]].c_str();
//                 str_len += strlen(res);
//             }
//             const char *res = idsLF[results[res_len - 3][line * res_len + res_len - 1]].c_str();
//             str_len += strlen(res);
//         }
//     }


//     //allocate space
//     int f_ret = ftruncate(fd, str_len);
//     if (-1 == f_ret)
//     {
//         printf("ftruncate fail\n");
//         exit(1);
//     }

//     char *const address = (char *)mmap(NULL, str_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

//     if (MAP_FAILED == address)
//     {
//         printf("mmap fail\n");
//         exit(1);
//     }

//     memcpy(address, buf, idx);

//     int pos = idx;

//     for (int res_len = 3; res_len <= 7; ++res_len)
//     {
//         for (int line = 0; line < num_res[res_len - 3]; ++line)
//         {
//             for (int id = 0; id < res_len - 1; ++id)
//             {
//                 const char *res = idsComma[results[res_len - 3][line * res_len + id]].c_str();
//                 memcpy(address + pos, res, strlen(res));
//                 pos += strlen(res);
//             }
//             const char *res = idsLF[results[res_len - 3][line * res_len + res_len - 1]].c_str();
//             memcpy(address + pos, res, strlen(res));
//             pos += strlen(res);
//         }
//     }

//     int mun_ret = munmap(address, str_len);
//     if (mun_ret == -1)
//     {
//         printf("munmap fail\n");
//         exit(1);
//     }
//     close(fd);

// #ifdef TEST
//     cout << "Output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
// #endif
// }