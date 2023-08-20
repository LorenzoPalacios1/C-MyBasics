#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "MyBasics.h"

// Set to non-zero to tell the program to use the specified seed.
// Otherwise, set to zero to let the program generate a seed.
#define MANUAL_SEED_SET 0

// The number of passes each test function should run.
#define DEFAULT_NUM_PASSES 5

// Determines the number of test strings to generate upon program start-up.
#define NUM_TEST_STRINGS 50

// Determines the base size for each test string to grow by.
// For each 10 strings generated, this value will be multiplied by 2, making the generated
// string's length equal to the expression (TEST_STRING_BASE_LENGTH * 2)
#define TEST_STRING_BASE_LENGTH 10

// Determines whether or not the test strings should use certain characters or not.
// Set to false, 0, to prevent the test strings from using ranges of characters, and non-zero
// to allow them.
#define TEST_STRINGS_USE_ALPHABET 1
#define TEST_STRINGS_USE_NUMERICAL 1
#define TEST_STRINGS_USE_INVIS_CHARS 0

// The maximum number of characters to be read by getStrStdin() when prompting the user
// for their test choices.
#define MAX_INPUT_PER_LINE 10

// The terminator to be used by tests_prompt() to signify the end of the returned int array.
// Preferably keep this value either greater than or less than NUM_TESTS.
#define USR_CHOICES_TERMINATOR -1

// Keyword for the user to input in order to select all of the available tests at once.
// Keyword length should be no longer than MAX_INPUT_PER_LINE.
#define RUN_ALL_TESTS_KEYWORD "ALL"

// The minimum signed char value for a visible character (inclusive).
#define VIS_CHAR_START (char)(' ' + 1)
// The maximum signed char value for a visible character (inclusive).
#define VIS_CHAR_END (char)('' - 1)

/* Contains strings generated at the start of the program for use in the testing functions below.
 *
 * This is done to make timing the tests more accurate, since generating the strings will be handled
 * at program start-up instead of during the tests themselves.
 *
 * Generic access to the strings should be done using get_test_string().
 * You can access the string by specific indexes if you want a specific string length, but you will have
 * to handle regenerating the strings yourself if you exhaust all of the available strings.
 *
 * Indexes 0-9 will have strings of length 10, indexes 10-19 will have strings of length 20,
 * indexes 20-29 will have strings of length 30, and so on and so forth.
 */
static char *test_strings[NUM_TEST_STRINGS];

/*
 * - Random Singleton Generators -
 */

/*
 * Returns a single int from rand() within the specified range (inclusive).
 * This function will always return 0 if (max < min).
 */
int random_int(const int min, const int max)
{
    if (max < min)
    {
        fprintf_s(stderr, "random_int(): Invalid values for max and min values (%u is NOT GREATER THAN %u)\n", max, min);
        return 0;
    }
    return rand() % (max - min + 1) + min;
}

/*
 * Returns either 0 or 1.
 */
unsigned char random_bool(void)
{
    return rand() % 2;
}

/*
 * Returns a single visible unsigned char from rand().
 */
unsigned char random_vis_uchar(void)
{
    return random_int(VIS_CHAR_START, UCHAR_MAX);
}

/*
 * Returns a single unsigned char from rand() within the min and max values (inclusive).
 */
unsigned char random_uchar_range(const unsigned char min, const unsigned char max)
{
    if (max < min)
    {
        fprintf_s(stderr, "random_uchar_range(): Invalid values for max and min values (%u is NOT GREATER THAN %u)\n", max, min);
        return 0;
    }
    return random_int(min, max);
}

/*
 * - Mass Random Generators -
 */

/*
 * Returns a random string of unsigned chars using random_vis_uchar().
 */
unsigned char *generate_test_ustring(const unsigned char min, const unsigned char max, const size_t length)
{
    if (max < min)
    {
        fprintf_s(stderr, "generate_test_ustring(): Invalid values for max and min values (%u is NOT GREATER THAN %u)\n", max, min);
        exit(EXIT_FAILURE);
        return 0;
    }

    if (length == 0)
    {
        fprintf_s(stderr, "generate_test_ustring(): Invalid length: %llu", length);
        return 0;
    }

    // (length + 1) to account for the null terminator
    unsigned char *str = malloc(sizeof(unsigned char) * (length + 1));

    if (str == NULL)
    {
        fputs("generate_test_ustring(): malloc() failure; returning NULL\n", stderr);
        return NULL;
    }

    // Writing the string
    for (size_t i = 0; i < length; i++)
        str[i] = random_uchar_range(min, max);

    str[length] = '\0';

    return str;
}

/*
 * Returns a random string of signed chars using random_vis_uchar().
 */
char *generate_test_string(const char min, const char max, const size_t length)
{
    if (max < min)
    {
        fprintf_s(stderr, "generate_test_ustring(): Invalid values for max and min values (%u is NOT GREATER THAN %u)\n", max, min);
        return 0;
    }

    if (length == 0)
    {
        fprintf_s(stderr, "generate_test_ustring(): Invalid length: %llu", length);
        return 0;
    }

    // (length + 1) to account for the null terminator
    char *str = malloc(sizeof(char) * (length + 1));
    if (str == NULL)
    {
        fputs("generate_test_string(): malloc() failure; returning NULL\n", stderr);
        return NULL;
    }

    // Writing the string
    for (size_t i = 0; i < length; i++)
        str[i] = random_uchar_range(min, max);

    str[length] = '\0';

    return str;
}

/*
 * Returns a random string of alphabetical chararacters using random_vis_uchar().
 */
char *generate_test_string_alphabetic(const size_t length)
{
    if (length == 0)
    {
        fprintf_s(stderr, "generate_test_string_alphabetic(): Invalid length: %llu", length);
        return 0;
    }

    // (length + 1) to account for the null terminator
    char *str = malloc(sizeof(char) * (length + 1));
    if (str == NULL)
    {
        fprintf_s(stderr, "generate_test_string(): malloc() failure; returning NULL\n");
        return NULL;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (random_bool())
            str[i] = random_uchar_range('a', 'z');
        else
            str[i] = random_uchar_range('A', 'Z');
    }
    str[length] = '\0';

    return str;
}

/*
 * Returns a pointer to a read-only FILE containing the specified amount of random characters.
 * This function will overwrite any files with the same name.
 */
FILE *generate_test_file(const char *filename, const size_t num_chars)
{
    FILE *NEW_TEST_FILE = NULL;

    const errno_t _fopen_errcode = fopen_s(&NEW_TEST_FILE, filename, "w");

    if (_fopen_errcode != 0)
    {
        fprintf_s(stderr, "generate_test_file(): Failed to open file %s\nError code: %d\n", filename, _fopen_errcode);
        return NULL;
    }

    // Here we fill the file with random gibberish
    for (unsigned i = 0; i < num_chars; i++)
    {
        fputc(random_vis_uchar(), NEW_TEST_FILE);
    }

    freopen_s(&NEW_TEST_FILE, filename, "r", NEW_TEST_FILE);
    return NEW_TEST_FILE;
}

/* Returns a pointer to a read-only FILE containing the specified amount of random characters
 * between min and max (inclusive).
 *
 * This function will overwrite any files with the same name.
 */
FILE *generate_test_file_range(const char *filename, const size_t num_chars, const unsigned char min, const unsigned char max)
{
    FILE *NEW_TEST_FILE = NULL;

    const errno_t _fopen_errcode = fopen_s(&NEW_TEST_FILE, filename, "w");

    if (_fopen_errcode != 0)
    {
        fprintf_s(stderr, "generate_test_file_range(): Failed to open file %s\nError code: %d\n", filename, _fopen_errcode);
        return NULL;
    }

    // Here we fill the file with random gibberish
    unsigned char buffer[num_chars + 1];
    for (unsigned i = 0; i < num_chars; i++)
    {
        buffer[i] = random_uchar_range(min, max);
    }
    buffer[num_chars] = '\0';
    fprintf_s(NEW_TEST_FILE, "%s", buffer);

    freopen_s(&NEW_TEST_FILE, filename, "r", NEW_TEST_FILE);
    return NEW_TEST_FILE;
}

/*
 * - Utility Functions -
 */

/* Generates a series of strings and assigns them to 'test_strings'.
 * This function does not use generate_test_string() or similar functions, but does use the
 * random_vis_char() function family.
 *
 * Indexes 0 - 9 will have strings of length 10, indexes 10 - 19 will have strings of length 20,
 * indexes 20 - 29 will have strings of length 30, and so on and so forth.
 */
void gen_test_strings(void)
{
    assert(NUM_TEST_STRINGS > 0);

    for (int i = 0; i < NUM_TEST_STRINGS; i++)
    {
        const size_t STR_SIZE = (i / 10 + 1) * TEST_STRING_BASE_LENGTH * sizeof(char);

        // STR_SIZE + sizeof(char) to account for the null terminator
        if (test_strings[i] == NULL) {
            test_strings[i] = malloc(STR_SIZE + sizeof(char));
        }

        // Allocation insurance
        assert(test_strings[i] != NULL);

        for (size_t j = 0; j < STR_SIZE; j++)
        {
            test_strings[i][j] = random_uchar_range(VIS_CHAR_START, VIS_CHAR_END);
        }
        test_strings[i][STR_SIZE] = '\0';
    }
}

/*
 * Retrieves a string from test_strings starting from 0 up to NUM_TEST_STRINGS -1.
 *
 * Upon all of the strings being exhausted, this function will regenerate the strings within test_strings,
 * then return the first string within test_strings.
 */
char *get_test_string(void)
{
    static size_t current_index = 0;

    if (current_index == NUM_TEST_STRINGS)
    {
        gen_test_strings();
        current_index = 0;
    }
    return test_strings[current_index++];
}

/*
 * Generates and applies a new seed for srand().
 * Returns the new seed.
 */
time_t random_seeder(void)
{
    if (MANUAL_SEED_SET == 0)
    {
        const time_t CUR_SEED = time(NULL) << 8;
        srand(CUR_SEED);
        return CUR_SEED;
    }
    else
    {
        srand(MANUAL_SEED_SET);
        return MANUAL_SEED_SET;
    }
}

/*
 * - Test Functions -
 */

static void fDiscardLineTests(void)
{
}

static void charToIntTests(void)
{
    for (int i = 0; i < DEFAULT_NUM_PASSES; i++)
    {
        const char *current_string = get_test_string();

        printf("\nRandom input %d: %s\n", i + 1, current_string);
        printf("\ncharToInt() output: ");

        for (unsigned j = 0; j < strlen(current_string); j++)
        {
            putc(charToInt(current_string[j]) + '0', stdout);
        }
    }
}

static void indexOfTests(void)
{
    for (int i = 0; i < DEFAULT_NUM_PASSES; i++)
    {
    }
}

// Array of function pointers for easy user access.
// Will have to be manually updated, but this shouldn't be too difficult to keep up with.
// Make sure to update 'NUM_TESTS' and 'test_names' accordingly.
static void (*const tests[])(void) = {charToIntTests, fDiscardLineTests, indexOfTests};
static const char *test_names[] = {"charToIntTests", "fDiscardLineTests", "indexOfTests"};

// The total number of tests available.
// (MAKE SURE TO INCREMENT THIS WHEN ADDING A NEW TEST FUNCTION)
#define NUM_TESTS 3

/*
 * Gives the user a generic prompt allowing them to choose from a list of available tests.
 * Returns an array of ints that correspond to the tests have been chosen, terminated by USR_CHOICES_TERMINATOR.
 */
int *tests_prompt(void)
{
    puts("Your test choices are:");
    for (int i = 0; i < NUM_TESTS; i++)
        printf_s("%d. %s\n", i, test_names[i]);

    printf_s("\nEnter the test number(s) you want to run, or \"%s\" (case insensitive) to run all of the tests.\n", RUN_ALL_TESTS_KEYWORD);
    puts("Press the enter/return key to start running the selected tests.");

    // This array will hold the indexes of the tests the user selects, but will use the value defined by USR_CHOICES_TERMINATOR
    // to define the end of the array, hence "(NUM_TESTS + 1)"
    int *choices = malloc(sizeof(int) * (NUM_TESTS + 1));
    if (choices == NULL)
    {
        fputs("\ntests_prompt(): malloc() failure", stderr);
        return NULL;
    }

    // Declaring the string outside of the loop means that it only has to be freed once.
    // This is fine because getStr() will be allocate memory once, and since getStr() will assume a non-null
    // pointer points to already allocated memory, we don't have to incur any new allocation costs because getStr()
    // will reuse the memory it has already allocated.
    char *input = malloc(MAX_INPUT_PER_LINE);
    int i = 0;
    for (; i < NUM_TESTS; i++)
    {
        input = getStrStdin(MAX_INPUT_PER_LINE);
        // - NEWLINE CHECK -

        // We exit upon reading a single newline which can be detected if getStrStdin() returns NULL
        // because getStr() will disregard input that consists only of the specified delimiter
        // character, hence causing it to not consider any more input
        if (input == NULL)
        {
            // Returning NULL if the user enters a single newline as their first "choice" since
            // there's no point in doing anything else with the memory allocated for 'choices'
            if (i == 0)
            {
                free(choices);
                return NULL;
            }
            break;
        }

        // - RUN ALL TESTS KEYWORD CHECK -

        // We overwrite any previous choices to give this a consistent behavior
        if (strcasecmp(input, RUN_ALL_TESTS_KEYWORD) == 0)
        {
            for (int j = 0; j < NUM_TESTS; j++)
            {
                choices[j] = j;
            }
            i = NUM_TESTS;
            break;
        }
        // - INTEGER PARSING -

        // If the user inputs non-numerical characters, a value >= 'NUM_TESTS', or a value less than 0, we
        // notify them and prevent the increment of 'i'
        if (strToInt(input, choices + i) == ERRCODE_SUCCESS && choices[i] < NUM_TESTS && choices[i] >= 0)
        {
            printf_s("Successfully added \"%s\" to the queue.\n", test_names[choices[i]]);
        }
        else
        {
            // "NUM_TESTS - 1" since 'NUM_TESTS' does not describe an array index, but the literal number of tests available
            printf_s("Invalid choice\nYour choice must be a whole number between 0-%d (inclusive)\n", NUM_TESTS - 1);
            // Negating the loop's incrementation so that the respective index of 'choices' has a chance to get a valid value
            i--;
        }
    }
    free(input);

    // Appending the necessary terminator to the array 'choices'
    if (i != NUM_TESTS)
        choices[i] = USR_CHOICES_TERMINATOR;
    else
        choices[NUM_TESTS] = USR_CHOICES_TERMINATOR;

    return choices;
}

int main(void)
{
    clock_t global_timer = clock();
    puts("\n - MyBasics Testing Suite - \n");

    // Ensuring that the seed for this test run is shown to the user so that ample information
    // is given for any future debugging
    printf_s("rand() Seed: %u\nrand() Sample: %d\n", random_seeder(), rand());

    // Generating the random strings for this session's run
    puts("Generating test strings...");
    gen_test_strings();
    printf_s("Generation completed in %ldms\n", clock() - global_timer);

    const clock_t user_input_timer = clock();
    const int *user_choices = tests_prompt();

    // We omit the amount of time the user took to input their choices from 'global_timer'
    global_timer += clock() - user_input_timer;

    if (user_choices == NULL)
        return ERRCODE_NULL_PTR;

    // Now we run the tests
    for (int i = 0; i < NUM_TESTS; i++)
    {
        if (user_choices[i] != USR_CHOICES_TERMINATOR)
        {
            printf_s("\nNow running %d - %s\n", user_choices[i], test_names[user_choices[i]]);
            const clock_t test_timer = clock();
            tests[user_choices[i]]();
            printf_s("\nFinished running %d - %s\nElapsed time: %ldms\n", user_choices[i], test_names[user_choices[i]], clock() - test_timer);
        }
        else
            break;
    }
    printf_s("\nTotal elapsed time: %ldms", clock() - global_timer);

    return ERRCODE_SUCCESS;
}
