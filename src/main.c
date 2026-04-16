#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifndef _WIN32
#include <getopt.h>
#endif
#include "server_time.h"
#include "steam_guard.h"
#include "mafile.h"

#ifndef _WIN32
static const struct option options[] = 
{
    {"help",    no_argument,       NULL, 'h'},
    {"align",   no_argument,       NULL, 'a'},
    {"input",   required_argument, NULL, 'i'},
    {"secret",  required_argument, NULL, 's'},
    {"no-line", no_argument,       NULL, 'n'}, // no trailing \n
    // @todo more options
    {NULL, 0, NULL, 0},
};
#endif

static struct {
    // params
    bool align;
    bool no_line;
    char *input_file;
    char *shared_secret;
    //
    int time_diff;
} self = {
    // params
    .align = false,
    .no_line = false,
    .input_file = NULL,
    .shared_secret = NULL,
    //
    .time_diff = 0,
};

void print_usage(char **argv)
{
    fprintf(
        stderr,
        "Usage: %s [-han] [-i maFile] [-s sharedSecret]\n"
    , argv[0]);
} // print_usage

int main(int argc, char **argv)
{
#ifdef _WIN32
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            print_usage(argv);
            exit(EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--align"))
        {
            self.align = true;
        }
        else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--no-line"))
        {
            self.no_line = true;
        }
        else if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input")) && i + 1 < argc)
        {
            self.input_file = argv[++i];
        }
        else if ((!strcmp(argv[i], "-s") || !strcmp(argv[i], "--secret")) && i + 1 < argc)
        {
            self.shared_secret = argv[++i];
        }
        else
        {
            fprintf(stderr, "Unknown option or missing argument: %s\n", argv[i]);
            print_usage(argv);
            exit(EXIT_FAILURE);
        }
    }
#else
    int opt;
    while ((opt = getopt_long(argc, argv, "hai:s:n", options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'h':
                print_usage(argv);
                exit(EXIT_SUCCESS);
            case 'a':
                self.align = true;
                break;
            case 'i':
                self.input_file = optarg;
                break;
            case 's':
                self.shared_secret = optarg;
                break;
            case 'n':
                self.no_line = true;
                break;
            case '?':
                fprintf(stderr, "Unknown option or missing argument: %c\n", optopt);
                print_usage(argv);
                exit(EXIT_FAILURE);
            default:
                break;
        } // end switch
    } // end while
#endif

    if (NULL == self.input_file && NULL == self.shared_secret)
    {
        fprintf(stderr, "Missing file or shared secret\n");
        exit(EXIT_FAILURE);
    }

    if (self.input_file)
    {
        mafile_obj *usr_data = read_mafile(self.input_file);
        self.shared_secret = strdup(usr_data->shared_secret);

        free_mafile_obj(usr_data);
    }

    if (self.align)
        self.time_diff = get_server_time_diff();

    char auth_code[6];
    if (gen_auth_code(auth_code, self.shared_secret, self.time_diff))
    {
        fprintf(stderr, "could not generate auth code\n");
        exit(EXIT_FAILURE);
    }

    printf("%s", auth_code);
    if (!self.no_line) putchar('\n');

    return EXIT_SUCCESS;
} // main
