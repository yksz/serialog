#include <stdio.h>
#include <stdlib.h>
#include "getopty.h"
#include "logger.h"
#include "serialevent.h"

const int kDefaultBaudRate = 9600;
const long kDefaultMaxFileSize = 10000000; /* 10MB */
const int kDefaultMaxBackupFiles = 10;

static char* s_output;

static void showUsage(const char* name)
{
    printf("Usage: %s [option] <portname> [baudrate]\n"
           "\n"
           "option:\n"
           "    -h        Show this help message\n"
           "    -o        Output the logs to a specified file\n"
           ""
           , name);
}

static int parse(int argc, char** argv, char** portname, int* baudrate)
{
    struct opty opty = {0};
    int opt;
    while ((opt = getopty(argc, argv, "ho:", &opty)) != -1) {
        switch (opt) {
            case 'h':
                showUsage(argv[0]);
                break;
            case 'o':
                s_output= opty.arg;
                break;
            default:
                break;
        }
    }

    if (argc - opty.ind <= 2) {
        showUsage(argv[0]);
        return 0; /* false */
    }
    *portname = argv[opty.ind + 0];
    *baudrate = atoi(argv[opty.ind + 1]);
    return 1; /* true */
}

static void onSerialDataReceived(SerialPort* serial)
{
    char buf[8192] = {0};
    int nbytes;

    nbytes = SerialPort_read(serial, buf, sizeof(buf) - 1);
    if (nbytes > 0) {
        LOG_INFO("%s", buf);
    }
}

int main(int argc, char** argv)
{
    SerialPort serial = {0};
    char* portname = NULL;
    int baudrate = 0;

    int ok = parse(argc, argv, &portname, &baudrate);
    if (!ok) {
        return 1;
    }
    if (baudrate <= 0) {
        baudrate = kDefaultBaudRate;
    }
    if (s_output == NULL) {
        logger_initConsoleLogger(stdout);
    } else {
        logger_initFileLogger(s_output, kDefaultMaxFileSize, kDefaultMaxBackupFiles);
    }

    serialevent_set(onSerialDataReceived);
    if (SerialPort_open(&serial, portname, baudrate) != 0) {
        LOG_ERROR("Failed to open serial port: '%s:%d'", portname, baudrate);
        return 1;
    }
    serialevent_add(&serial);
    serialevent_start();
    SerialPort_close(&serial);
    return 0;
}
