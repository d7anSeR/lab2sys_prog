#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <cerrno>

using namespace std;

const int MAX_MESSAGE_SIZE = 256;
const int MAX_GUESSES = 50;

// Функция первого игрока
void firstPlayer(int N, mqd_t mq) {
    srand(time(NULL));
    int secretNumber = rand() % N + 1;
    cout << "Загаданное число: " << secretNumber << endl;
    // Отправляем секретное число в очередь сообщений
    char buffer[MAX_MESSAGE_SIZE];
    sprintf(buffer, "%d", secretNumber);
    if (mq_send(mq, buffer, strlen(buffer) + 1, 0) == -1) {
        cerr << "Ошибка при отправке сообщения" << endl;
    }
    
}

// Функция второго игрока
void secondPlayer(int N, mqd_t mq) {
    int guess = 0;
    int attempts = 0;
    int secretNumber = 0;
    std::stringstream ss;

    // Получаем секретное число из очереди сообщений
    char buffer[MAX_MESSAGE_SIZE] = "";
    ssize_t bytes = 0;
    bytes = mq_receive(mq, buffer, MAX_MESSAGE_SIZE, NULL);
    if(bytes < 0){
        cerr << "Ошибка при приеме сообщения: " << strerror(errno) << endl;
    }
    else{
        cout << "Сообщение было принято вторым процессом "<< endl;
        secretNumber = atoi(buffer);
        cout << secretNumber << endl;
        // Играем, пока не угадаем число или не достигнем максимального числа попыток
        do {
            guess = rand() % N + 1;
            cout << "Попытка: " << guess << endl;
            attempts++;
        } while (guess != secretNumber && attempts < MAX_GUESSES);
        // Выводим статистику игры
        if (guess == secretNumber) {
            cout << "Число попыток: " << attempts << endl;
        } else {
            cout << "Не удалось угадать число за " << MAX_GUESSES << " попыток." << endl;
        }
    }    
}

int main(int argc, char *argv[]) {
    int N = 3;
    pid_t pid = fork();
    if (pid) {
        mqd_t mq = mq_open("/queue", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        // Второй процесс (второй игрок)
        cout << mq << endl;
        secondPlayer(N, mq);
        int stat;
        wait(&stat);
        mq_close(mq);
        mq_unlink("/queue");
    }
    else{
        mqd_t mq = mq_open("/queue", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        cout << mq << endl;
        firstPlayer(N, mq);
        mq_close(mq);
        mq_unlink("/queue");
    }

    return EXIT_SUCCESS;
}