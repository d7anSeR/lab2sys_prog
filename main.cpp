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
    char buffer[MAX_MESSAGE_SIZE ];
    mq_receive(mq, buffer, MAX_MESSAGE_SIZE, NULL);
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Использование: " << argv[0] << " N" << endl;
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);

    // Создаем очередь сообщений
    mqd_t mq = mq_open("/number_guessing_queue", O_RDWR | O_CREAT, 0600, NULL);
    if (mq == (mqd_t)-1) {
        cerr << "Ошибка при создании очереди сообщений" << endl;
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid < 0) {
        cerr << "Ошибка при создании второго процесса" << endl;
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // Второй процесс (второй игрок)
        secondPlayer(N, mq);
    } else {
        // Первый процесс (первый игрок)
        firstPlayer(N, mq);

        // Ждем завершения второго процесса
        wait(NULL);

        // Закрываем и удаляем очередь сообщений
        mq_close(mq);
        mq_unlink("/number_guessing_queue");
    }

    return EXIT_SUCCESS;
}