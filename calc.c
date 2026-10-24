// Daniels Dolgins 251RDC081
// https://github.com/danielsdolgins-ship-it/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <https://www.w3schools.com/, https://www.programiz.com/>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_PATH 512

static long global_error_pos = -1; 

void set_error(long pos) {
    if (global_error_pos == -1) {   // saves the first error, ignores errors after
        global_error_pos = pos;
    }
}

static long long parse_expr(const char *line, int *idx, long line_offset);
static long long parse_term(const char *line, int *idx, long line_offset);      // declare functions
static long long parse_power(const char *line, int *idx, long line_offset);
static long long parse_primary(const char *line, int *idx, long line_offset);
//https://www.programiz.com/c-programming/library-function/ctype.h/isspace
static void skip_spaces(const char *line, int *idx) {
    while (line[*idx] == ' ' || line[*idx] == '\t') {   //to skip spaces or tabs
        (*idx)++;
    }
}

static long long parse_primary(const char *line, int *idx, long line_offset) {      //reads the math problem or a single number
    skip_spaces(line, idx);

    if (global_error_pos != -1) return 0;       // if error, stops

    if (line[*idx] == '(') {                            // if a ( appears we evaluate whats inside it
        (*idx)++;       
        long long val = parse_expr(line, idx, line_offset);
        
        skip_spaces(line, idx);
        if (global_error_pos != -1) return 0;

        if (line[*idx] == ')') {
            (*idx)++; 
            return val;
        } else {
            set_error(line_offset + *idx + 1);      // if no ) appears, error
            return 0;
        }
    }
    //https://www.programiz.com/c-programming/library-function/ctype.h/isdigit
    if (isdigit((unsigned char)line[*idx])) {       // if we find a digit, turn the text digits into a full integer
        long long num = 0;
        while (isdigit((unsigned char)line[*idx])) {
            num = num * 10 + (line[*idx] - '0');        // build multi digit number, like "2" "5" will be 25
            (*idx)++;
        }
        return num;
    }
    set_error(line_offset + *idx + 1);      // error if we dont get number or ()
    return 0;
}

static long long parse_power(const char *line, int *idx, long line_offset) {        // does exponents so 3 ** 3 = 27
    long long left = parse_primary(line, idx, line_offset);     // gets base number
    if (global_error_pos != -1) return 0;

    skip_spaces(line, idx);

    if (line[*idx] == '*' && line[*idx + 1] == '*') {       // sees if the current and next char is **
        (*idx) += 2; 
        skip_spaces(line, idx);

        long exp_pos = line_offset + *idx + 1;
        long long right = parse_power(line, idx, line_offset); // recursive calls for right associativity
        if (global_error_pos != -1) return 0;

        if (right < 0) {
            set_error(exp_pos);     // no negative exponents
            return 0;
        }

        long long result = 1;
        for (long long i = 0; i < right; i++) {
            result *= left;
        }
        return result;
    }

    return left;
}

// * and / math problems
static long long parse_term(const char *line, int *idx, long line_offset) {
    long long left = parse_power(line, idx, line_offset);       // * and / have less priority than ** we call parse_power
    if (global_error_pos != -1) return 0;

    while (1) {
        skip_spaces(line, idx);
        char op = line[*idx];
        if (op == '*' && line[*idx + 1] != '*') {   //check for *, not **
            (*idx)++;
            long long right = parse_power(line, idx, line_offset);
            if (global_error_pos != -1) return 0;
            left = left * right;
        } else if (op == '/') {
            (*idx)++;
            skip_spaces(line, idx);
            
            long right_pos = line_offset + *idx + 1;
            long long right = parse_power(line, idx, line_offset);
            if (global_error_pos != -1) return 0;

            if (right == 0) {       // division by zero error
                set_error(right_pos);
                return 0;
            }
            left = left / right;
        } else {
            break;      // exit loop when no * or / found
        }
    }

    return left;
}

// + and - math problems
static long long parse_expr(const char *line, int *idx, long line_offset) {
    long long left = parse_term(line, idx, line_offset);            // * and / have priority call parse_term
    if (global_error_pos != -1) return 0;

    while (1) {
        skip_spaces(line, idx);
        char op = line[*idx];

        if (op == '+' || op == '-') {
            (*idx)++;
            long long right = parse_term(line, idx, line_offset);
            if (global_error_pos != -1) return 0;

            if (op == '+') {
                left = left + right;
            } else {
                left = left - right;
            }
        } else {
            break;          // exit when + and - not found
        }
    }

    return left;
}

// checks one line for comments or errors, and does the math answer
static int eval_line(const char *line, long line_offset, long long *result) {
    int idx = 0;
    skip_spaces(line, &idx);

    if (line[idx] == '\0' || line[idx] == '\n' || line[idx] == '\r') {  // ignores blank lines
        return 0;
    }

    if (line[idx] == '#') {                 // ignores comments starting with #
        return 0;
    }

    for (int i = 0; line[i] != '\0' && line[i] != '\n' && line[i] != '\r'; i++) {   // checks invalid characters
        if (line[i] == '#') {
            set_error(line_offset + i + 1);     // error
            return -1;
        }
    }

    long long val = parse_expr(line, &idx, line_offset);        // runs the math parser
    if (global_error_pos != -1) return -1;

    skip_spaces(line, &idx);

    if (line[idx] != '\0' && line[idx] != '\n' && line[idx] != '\r') {      // checks for junk characters at the end of the line
        set_error(line_offset + idx + 1);
        return -1;
    }

    *result = val;
    return 1;
}
//  reads input file and writes the asnwers, or error https://www.w3schools.com/c/c_files.php
static void process_file(const char *input_path, const char *output_path) {
    FILE *in = fopen(input_path, "r");
    if (!in) {
        fprintf(stderr, "Error opening input file: %s\n", input_path);
        exit(1);
    }

    global_error_pos = -1;  // reset error tracker for file

    // reads full file into memory
    size_t cap = 1024;
    size_t len = 0;
    char *buf = malloc(cap);

    int c;
    while ((c = fgetc(in)) != EOF) {
        buf[len++] = (char)c;
        if (len >= cap - 1) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }
    buf[len] = '\0';
    fclose(in);

    long line_start = 0;

    // 1st pass to find if any error occurs
    for (long i = 0; i <= (long)len; i++) {
        if (buf[i] == '\n' || buf[i] == '\0') {
            char line_buf[2048];
            long line_len = i - line_start;
            if (line_len >= (long)sizeof(line_buf)) line_len = sizeof(line_buf) - 1;

            strncpy(line_buf, buf + line_start, line_len);
            line_buf[line_len] = '\0';

            long long val = 0;
            eval_line(line_buf, line_start, &val);

            if (global_error_pos != -1) break;  //error stops it

            line_start = i + 1;
            if (buf[i] == '\0') break;
        }
    }

    FILE *out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "Error opening output file: %s\n", output_path);
        free(buf);
        exit(1);
    }

    // if error occurs, write "ERROR:<pos>" and exits
    if (global_error_pos != -1) {
        fprintf(out, "ERROR:%ld\n", global_error_pos);
        fclose(out);
        free(buf);
        return;
    }

    // 2nd pass, if no errors write results
    line_start = 0;
    for (long i = 0; i <= (long)len; i++) {
        if (buf[i] == '\n' || buf[i] == '\0') {
            char line_buf[2048];
            long line_len = i - line_start;
            if (line_len >= (long)sizeof(line_buf)) line_len = sizeof(line_buf) - 1;

            strncpy(line_buf, buf + line_start, line_len);
            line_buf[line_len] = '\0';

            long long val = 0;
            int status = eval_line(line_buf, line_start, &val);
            if (status == 1) {      //validate
                fprintf(out, "%lld\n", val);
            }

            line_start = i + 1;
            if (buf[i] == '\0') break;
        }
    }

    fclose(out);
    free(buf);
}
// read command line arguments and run file procesor
int main(int argc, char *argv[]) {
    const char *out_dir = "results";    // folder for output files
    const char *in_file = NULL;
    // https://www.programiz.com/c-programming/library-function/string.h/strcmp
    //check for options like -o for changing output directory
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_dir = argv[++i];    //custom folder
        } else {
            in_file = argv[i];  // file name
        }
    }

    if (!in_file) {
        fprintf(stderr, "No input file provided.\n");
        return 1;
    }
    mkdir(out_dir, 0755);   // create output folder if one does not exist

    char out_path[MAX_PATH];
    snprintf(out_path, sizeof(out_path), "%s/task1_daniels_dolgins_251RDC081.txt", out_dir);

    process_file(in_file, out_path);

    return 0;
}