#include "check.hpp"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <mqueue.h> 
void lead(mqd_t send, mqd_t get, const char* procname, int N)
{
    std::cout << procname << ": I guessed a number from 1 to "<< N <<". Try to guess it!" << std::endl;
    srand(time(nullptr));
    int number = rand() % N + 1;
    int response = 2;
    check(mq_send(send, (const char*)&response, sizeof(response), 0));
    int attempt;
    int count = 0;
    double time1 = clock();
    do
    {
        count++;
        while(true)
        {
            check(mq_receive(get, (char*)&attempt, sizeof(attempt), nullptr)); 
            if (attempt > 0 && attempt < 11)
                break;
        }

        if (attempt != number)
        {
            response = 0;
            std::cout << procname << ": Nope, try again!" << std::endl;
        }
        else
        {
            response = 1;
            double time2 = clock();
            double time_c = time2 - time1;
            std::cout << procname << ": YEP, that's it!" << std::endl;
            std::cout << "Attempts: " << count << "\t Time: " << time_c << " msc "<<std::endl << std::endl;
        }
        
        check(mq_send(send, (const char*)&response, sizeof(response), 0)); 

    } while(attempt != number);
}

void guess(mqd_t send, mqd_t get, const char* procname, int N)
{
    sleep(2);
    int response;
    while(true)
    {
        check(mq_receive(get, (char*)&response, sizeof(response), nullptr)); 
        if (response == 2)
            break;
    }
    std::vector<int> used;
    while(response != 1)
    {
        int attempt;
        do
        {
            srand(time(nullptr));
            attempt = rand() % N + 1;

            if (std::find(used.begin(), used.end(), attempt) == used.end())
            {
                used.push_back(attempt);
                break;
            }
        }while(std::find(used.begin(), used.end(), attempt) != used.end());

        std::cout << procname << ": Maybe it's " << attempt << "?" << std::endl;
        check(mq_send(send, (const char*)&attempt, sizeof(attempt), 0)); 
        while(true)
        {
            check(mq_receive(get, (char*)&response, sizeof(response), nullptr)); 
            if (response == 1 or response == 0)
                break;
        }
    }
}

int main(int argv, char* argc[])
{
    int iterations = 0;
    int N = 0;
    if (argc[1] != 0)
    {
        N = std::strtol(argc[1], nullptr, 10);
        if (N < 0)
            N = -1*N;
    }
    else
        N = 3;

    if (argc[2] != 0)
    {
        iterations = std::strtol(argc[2], nullptr, 10);
        if (iterations < 0)
            iterations = -1*iterations;
    }
    else
        iterations = 2;

    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Range: " << N << std::endl;
    bool leads = 1;
    mqd_t a_mq, b_mq;
    struct mq_attr attr; 
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);

    pid_t pid = check(fork());
    for (int i = 0; i < iterations; i++)
    {

        if(pid)
        {
            a_mq = mq_open("/queuea", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr); 
            b_mq = mq_open("/queueb", O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
            if (leads)
                lead(a_mq, b_mq, "proc1", N);
            else
                guess(a_mq, b_mq, "proc1", N);
        }
        else
        {
            a_mq = mq_open("/queuea", O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
            b_mq = mq_open("/queueb", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
            if (!leads)
                lead(b_mq, a_mq, "proc2", N);
            else
                guess(b_mq, a_mq, "proc2", N);
        }
        leads = !leads;
        mq_close(a_mq); 
        mq_close(b_mq);
    }
    mq_unlink("/queuea");
    mq_unlink("/queueb");
    return 0;
}